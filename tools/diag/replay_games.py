#!/usr/bin/env python3
"""One side of a Colosseum PGN replayed at its own clocks, one row per search.

Each game gets one engine process: `ucinewgame`, then for every move the named
side played, `position ... moves ...` and `go` with both clocks rebuilt from the
PGN's `t=` comments and the header's control. `TM_Debug` and `Diag` are on, so a
row holds the budget the engine set, what the search spent and the counters of
the fail-high stop's firing read (the aspiration counters, `fhstop_fired`, the
ply-cap counters, the longest window's start and length).

The game is replayed, not played: the engine's own move is thrown away and the
game's next position is searched, so every arm reads the same positions at the
same clocks. Nothing is judged here; a consumer aggregates the rows.

The engine must advertise `Diag`: a Diag or Tune build. Every `--option` is sent
after `uci`, so one binary with `B5FailHighStop` set gives the three arms of a
firing read (a Tune build advertises it; a Diag build does not).

Usage:
  python tools/diag/replay_games.py --engine tune.exe --pgn games.pgn --side basilisk-b47-on \
      --games 300 --out rows.jsonl [--option B5FailHighStop=200]
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys
import time

try:
    import chess
    import chess.pgn
except ImportError:  # pragma: no cover
    sys.exit("python-chess is required: pip install chess")

CONTROL = re.compile(r"white (\d+)ms\+(\d+)ms; black (\d+)ms\+(\d+)ms")
COMMENT_TIME = re.compile(r"\bt=(\d+)ms")
INFO_SCORE = re.compile(r"\bscore (cp|mate) (-?\d+)")
INFO_DEPTH = re.compile(r"^info depth (\d+)\b")
TM_LINE = re.compile(r"soft_ms=(-?\d+) hard_ms=(-?\d+) elapsed_ms=(-?\d+) dispatch_ms=(-?\d+)"
                     r" longest_window_start_ms=(-?\d+) longest_window_ms=(-?\d+)")
KV_TOKEN = re.compile(r"(\w+)=(-?\d+)")
COUNTERS = ("asp_windows", "asp_fail_low", "asp_fail_high", "asp_researches", "asp_giveup",
            "asp_nodes_windowed", "asp_nodes_failed", "asp_nodes_giveup",
            "asp_fail_high_research_nodes", "fhstop_fired", "ply_cap_hits", "ply_cap_in_check",
            "qs_ply_cap_hits", "qs_ply_cap_in_check", "root_effort_hi", "root_effort_lo",
            "interior_nodes", "qs_nodes")


def parse_control(header: str) -> tuple[dict[bool, int], dict[bool, int]]:
    """Both sides' starting clocks and increments, in milliseconds."""
    match = CONTROL.search(header)
    if not match:
        raise ValueError(f"cannot read the time control {header!r}")
    white_base, white_inc, black_base, black_inc = map(int, match.groups())
    return ({chess.WHITE: white_base, chess.BLACK: black_base},
            {chess.WHITE: white_inc, chess.BLACK: black_inc})


def rebuild_clocks(game: chess.pgn.Game) -> list[dict]:
    """Every move with both clocks as they stood when it was searched.

    A move's comment holds what it spent. The mover's clock falls by that and
    gains the increment, as the harness that wrote the comment charged it.
    """
    clock, inc = parse_control(game.headers["TimeControl"])
    board = game.board()
    moves: list[str] = []
    rows = []
    for node in game.mainline():
        match = COMMENT_TIME.search(node.comment or "")
        if match is None:
            raise ValueError(f"move {len(moves)} of round {game.headers.get('Round')} has no t=")
        spent = int(match.group(1))
        side = board.turn
        rows.append({"side": side, "moves": list(moves), "san": board.san(node.move), "spent": spent,
                     "wtime": clock[chess.WHITE], "btime": clock[chess.BLACK],
                     "winc": inc[chess.WHITE], "binc": inc[chess.BLACK]})
        clock[side] = clock[side] - spent + inc[side]
        moves.append(node.move.uci())
        board.push(node.move)
    return rows


class Engine:
    def __init__(self, path: pathlib.Path, options: list[str], hash_mb: int) -> None:
        self.proc = subprocess.Popen([str(path)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     stderr=subprocess.DEVNULL, text=True, bufsize=1)
        self.send("uci")
        advertised = self.read_until("uciok")
        names = [opt.split("=", 1)[0].lower() for opt in options] + ["diag", "tm_debug"]
        text = "\n".join(advertised).lower()
        for name in names:
            if f"option name {name} " not in text:
                raise SystemExit(f"{path}: the engine does not advertise {name}")
        self.send("setoption name Threads value 1")
        self.send(f"setoption name Hash value {hash_mb}")
        self.send("setoption name Diag value true")
        self.send("setoption name TM_Debug value true")
        for opt in options:
            name, value = opt.split("=", 1)
            self.send(f"setoption name {name} value {value}")

    def send(self, line: str) -> None:
        assert self.proc.stdin is not None
        self.proc.stdin.write(line + "\n")
        self.proc.stdin.flush()

    def read_until(self, token: str) -> list[str]:
        assert self.proc.stdout is not None
        lines = []
        for line in self.proc.stdout:
            lines.append(line.rstrip())
            if line.startswith(token):
                return lines
        raise SystemExit("the engine closed its output")

    def new_game(self) -> None:
        self.send("ucinewgame")
        self.send("isready")
        self.read_until("readyok")

    def close(self) -> int:
        self.send("quit")
        self.proc.wait(timeout=30)
        return self.proc.returncode


def search_row(engine: Engine, root_fen: str, row: dict) -> dict:
    """One search at the row's clocks. The diagnostic and tm lines print after
    `bestmove` on some paths, so `isready` closes the read."""
    moves = " ".join(row["moves"])
    engine.send(f"position fen {root_fen}" + (f" moves {moves}" if moves else ""))
    began = time.perf_counter()
    engine.send(f"go wtime {row['wtime']} btime {row['btime']} winc {row['winc']} binc {row['binc']}")
    lines = engine.read_until("bestmove")
    wall_ms = (time.perf_counter() - began) * 1000.0
    engine.send("isready")
    lines += engine.read_until("readyok")

    out: dict = {"wall_ms": round(wall_ms, 1), "score": None, "depth": 0}
    counters: dict[str, int] = {}
    tm = None
    for line in lines:
        depth = INFO_DEPTH.match(line)
        if depth:
            out["depth"] = int(depth.group(1))
            score = INFO_SCORE.search(line)
            if score:
                out["score"] = f"{score.group(1)} {score.group(2)}"
        if "tm soft_ms=" in line:
            tm = TM_LINE.search(line)
        elif "kv " in line:
            for name, value in KV_TOKEN.findall(line):
                counters[name] = int(value)
    if tm is None:
        raise SystemExit(f"no tm line from the engine: {lines[-3:]}")
    optimum, maximum, elapsed, _, window_start, window_len = map(int, tm.groups())
    out.update(optimum=optimum, maximum=maximum, elapsed=elapsed,
               longest_window_start=window_start, longest_window=window_len)
    for name in COUNTERS:
        if name not in counters:
            raise SystemExit(f"counter {name} is missing: the engine needs the Cluster 4 counters")
        out[name] = counters[name]
    return out


def replay(args: argparse.Namespace) -> int:
    done = 0
    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.pgn.open(encoding="utf-8", errors="replace") as pgn, \
            args.out.open("w", encoding="utf-8") as sink:
        round_no = 0
        while done < args.games:
            game = chess.pgn.read_game(pgn)
            if game is None:
                break
            round_no += 1
            sides = {chess.WHITE: game.headers["White"], chess.BLACK: game.headers["Black"]}
            if not any(args.side in name for name in sides.values()):
                continue
            rows = rebuild_clocks(game)
            root_fen = game.board().fen()
            engine = Engine(args.engine, args.option, args.hash)
            engine.new_game()
            for row in rows:
                if args.side not in sides[row["side"]]:
                    continue
                searched = search_row(engine, root_fen, row)
                is_white = row["side"] == chess.WHITE
                sink.write(json.dumps({
                    "game": round_no, "ply": len(row["moves"]), "san": row["san"],
                    "clock": row["wtime"] if is_white else row["btime"],
                    "increment": row["winc"] if is_white else row["binc"],
                    "game_spent": row["spent"], **searched}) + "\n")
            sink.flush()
            code = engine.close()
            if code != 0:
                print(f"the engine exited {code} on game {round_no}")
                return 1
            done += 1
            print(f"game {round_no} done ({done} of {args.games})", flush=True)
    if done < args.games:
        print(f"only {done} games of {args.games} for side {args.side!r}")
        return 1
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--engine", required=True, type=pathlib.Path)
    parser.add_argument("--pgn", required=True, type=pathlib.Path)
    parser.add_argument("--side", required=True, help="substring of the replayed side's name")
    parser.add_argument("--games", required=True, type=int)
    parser.add_argument("--out", required=True, type=pathlib.Path)
    parser.add_argument("--hash", type=int, default=64)
    parser.add_argument("--option", action="append", default=[], help="Name=Value, repeatable")
    return replay(parser.parse_args())


if __name__ == "__main__":
    sys.exit(main())
