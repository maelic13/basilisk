#!/usr/bin/env python3
"""Where Basilisk loses: every move of stored games scored by a judge engine.

A judge (Stockfish 19 at a fixed node count, one thread, its table cleared at
every game) scores every position of a game from the side to move. A move's
loss is the judge's score before it plus the opponent's score after it (the
mover's score after, negated), kept between 0 and 300 cp: below about 30 cp it
is the judge's noise, above 300 the game is decided. Moves are tabulated by
the mover (ours, or the opponent as the control), the position's class by the
judge's score before the move (defending at -50 or below, ahead at +50 or
above, level between), the share of the base clock left before the move, and
the men on the board.

Modes:
  judge     score the games of one PGN source; resumable: games already in
            <out>/<source>.jsonl are skipped
  tables    the cells of every judged source: moves, mean loss, the share of
            moves losing 50 cp or more and the share of the side's total loss
  research  the worst errors of ours searched again by the binary that played
            them, at 1, 4 and 16 times the nodes it used in the game, each new
            move scored by the judge: an error the deeper searches mend is a
            matter of depth (the clock), one that stays is the evaluation's

Clock: rebuilt from the TimeControl header and each move's `t=` annotation
(Colosseum's `{s= d= t= n=}` comments): the increment is added after each
move. A source without annotations has no clock cells and no re-search.

Usage:
  python tools/diag/loss_by_cell.py judge --pgn P --source NAME --ours REGEX \
      --judge SF.exe --nodes 150000 --workers 14 --out DIR [--first N] [--only REGEX]
  python tools/diag/loss_by_cell.py tables --out DIR
  python tools/diag/loss_by_cell.py research --out DIR --source NAME \
      --engine "PLAYER=PATH" ... --judge SF.exe --nodes 150000 --top 100 --workers 14
"""
from __future__ import annotations

import argparse
import concurrent.futures as futures
import ctypes
import io
import json
import multiprocessing
import os
import pathlib
import re
import subprocess
import sys
import time

import chess
import chess.pgn

CAP = 300                 # a loss is counted up to this many cp
ERROR = 50                # a move losing this much or more is an error
CLASS_EDGE = 50           # defending at -50 or below, ahead at +50 or above
MATE = 10000              # a mate in n scores MATE - n
ANNOTATION = re.compile(r"\b([sdtn])=(-?\d+)(?:ms)?")
TC_SIMPLE = re.compile(r"^(\d+(?:\.\d+)?)\+(\d+(?:\.\d+)?)$")
TC_SIDES = re.compile(r"white (\d+)ms\+(\d+)ms; black (\d+)ms\+(\d+)ms")


# ---- Scores and cells --------------------------------------------------------

def value(score: dict) -> int:
    """A judge score as one number: cp, or MATE - n for a mate in n."""
    if score.get("mate") is not None:
        m = score["mate"]
        return -MATE if m == 0 else (MATE - m if m > 0 else -MATE - m)
    return int(score["cp"])


def move_loss(before: dict, after: dict) -> int:
    """The mover's loss, uncapped: its score before less its score after."""
    return value(before) + value(after)


def capped(loss: int) -> int:
    return max(0, min(CAP, loss))


def position_class(before: dict) -> str:
    v = value(before)
    return "defending" if v <= -CLASS_EDGE else "ahead" if v >= CLASS_EDGE else "level"


def clock_band(clock_ms: int | None, base_ms: int | None) -> str:
    if clock_ms is None or not base_ms:
        return "unknown"
    share = clock_ms / base_ms
    return "<10%" if share < 0.10 else "10-30%" if share < 0.30 else "30%+"


def material_band(men: int) -> str:
    return "24+" if men >= 24 else "16-23" if men >= 16 else "10-15" if men >= 10 else "<=9"


def time_control(headers) -> tuple[int, int, int, int] | None:
    """(white base, white increment, black base, black increment) in ms."""
    tc = headers.get("TimeControl", "")
    m = TC_SIDES.search(tc)
    if m:
        return tuple(int(x) for x in m.groups())  # type: ignore[return-value]
    m = TC_SIMPLE.match(tc.strip())
    if m:
        base, inc = round(float(m.group(1)) * 1000), round(float(m.group(2)) * 1000)
        return base, inc, base, inc
    return None


def parse_annotation(comment: str) -> dict | None:
    fields = {k: int(v) for k, v in ANNOTATION.findall(comment or "")}
    return fields if "t" in fields else None


def clocks(meta: list[dict | None], first_white: bool,
           tc: tuple[int, int, int, int] | None) -> list[int | None]:
    """Each mover's clock before its move, in ms (None without annotations)."""
    if tc is None or any(m is None for m in meta):
        return [None] * len(meta)
    left = {True: tc[0], False: tc[2]}
    inc = {True: tc[1], False: tc[3]}
    out = []
    white = first_white
    for m in meta:
        out.append(left[white])
        left[white] += inc[white] - m["t"]  # type: ignore[index]
        white = not white
    return out


# ---- Games -------------------------------------------------------------------

def read_games(pgn: pathlib.Path, only: str | None, first: int):
    """Yield (index, game) for the games to judge: those with a player matching
    `only` (all when None), the first `first` of them (all when 0)."""
    wanted = re.compile(only) if only else None
    taken = 0
    with pgn.open(encoding="utf-8", errors="replace") as handle:
        index = 0
        while True:
            offset = handle.tell()
            headers = chess.pgn.read_headers(handle)
            if headers is None:
                return
            index += 1
            if wanted and not (wanted.search(headers.get("White", ""))
                               or wanted.search(headers.get("Black", ""))):
                continue
            handle.seek(offset)
            game = chess.pgn.read_game(handle)
            yield index, game
            taken += 1
            if first and taken >= first:
                return


def game_record(source: str, index: int, game) -> dict:
    board = game.board()
    fen = board.fen()
    moves, meta, men = [], [], []
    for node in game.mainline():
        men.append(chess.popcount(board.occupied))
        moves.append(node.move.uci())
        meta.append(parse_annotation(node.comment))
        board.push(node.move)
    tc = time_control(game.headers)
    first_white = game.board().turn == chess.WHITE
    return {"source": source, "index": index, "white": game.headers.get("White", "?"),
            "black": game.headers.get("Black", "?"), "result": game.headers.get("Result", "*"),
            "termination": game.headers.get("Termination", ""), "fen": fen, "moves": moves,
            "meta": meta, "men": men, "first_white": first_white,
            "base_ms": None if tc is None else [tc[0], tc[2]],
            "clock_ms": clocks(meta, first_white, tc)}


# ---- UCI engines -------------------------------------------------------------

class Uci:
    def __init__(self, path: str, options: dict[str, str], cpu: int | None):
        self.proc = subprocess.Popen([path], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                     stderr=subprocess.DEVNULL, text=True, bufsize=1)
        if cpu is not None and os.name == "nt":
            kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
            kernel32.SetProcessAffinityMask.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
            kernel32.SetProcessAffinityMask(ctypes.c_void_p(int(self.proc._handle)),  # type: ignore[attr-defined]
                                            1 << cpu)
        self.send("uci")
        self.until("uciok")
        for name, val in options.items():
            self.send(f"setoption name {name} value {val}")
        self.send("isready")
        self.until("readyok")

    def send(self, line: str) -> None:
        self.proc.stdin.write(line + "\n")
        self.proc.stdin.flush()

    def until(self, prefix: str) -> list[str]:
        lines = []
        while True:
            line = self.proc.stdout.readline()
            if not line:
                raise RuntimeError(f"engine exited waiting for {prefix}")
            line = line.strip()
            lines.append(line)
            if line.startswith(prefix):
                return lines

    def new_game(self) -> None:
        self.send("ucinewgame")
        self.send("isready")
        self.until("readyok")

    def search(self, fen: str, moves: list[str], nodes: int) -> tuple[dict, str]:
        """The last exact score (side to move) and the bestmove."""
        self.send(f"position fen {fen}" + (" moves " + " ".join(moves) if moves else ""))
        self.send(f"go nodes {nodes}")
        lines = self.until("bestmove")
        score = None
        for line in lines:
            if not line.startswith("info ") or " score " not in line:
                continue
            tokens = line.split()
            i = tokens.index("score")
            bound = len(tokens) > i + 3 and tokens[i + 3] in ("lowerbound", "upperbound")
            if score is not None and bound:
                continue
            score = {"cp": int(tokens[i + 2])} if tokens[i + 1] == "cp" \
                else {"mate": int(tokens[i + 2])}
        best = lines[-1].split()[1]
        if score is None:
            raise RuntimeError(f"no score from {fen} {moves}")
        return score, best

    def close(self) -> None:
        try:
            self.send("quit")
            self.proc.wait(timeout=10)
        except (OSError, subprocess.TimeoutExpired):
            self.proc.kill()


def terminal_score(board: chess.Board) -> dict | None:
    """A position the judge cannot search: mate, or a draw by the board."""
    if board.is_checkmate():
        return {"mate": 0}
    if board.is_stalemate() or board.is_insufficient_material():
        return {"cp": 0}
    return None


_worker: dict = {}


def _init_worker(judge: str, cpus, lock) -> None:
    with lock:
        cpu = cpus.pop(0) if len(cpus) else None
    _worker["judge"] = Uci(judge, {"Threads": "1", "Hash": "16"}, cpu)
    _worker["cpu"] = cpu


def _judge_game(record: dict, nodes: int) -> dict:
    engine: Uci = _worker["judge"]
    engine.new_game()
    board = chess.Board(record["fen"])
    scores = []
    for k in range(len(record["moves"]) + 1):
        fixed = terminal_score(board)
        if fixed is None:
            fixed, _ = engine.search(record["fen"], record["moves"][:k], nodes)
        scores.append(fixed)
        if k < len(record["moves"]):
            board.push_uci(record["moves"][k])
    record["judge"] = scores
    record["judge_nodes"] = nodes
    return record


def run_judge(args) -> int:
    out = pathlib.Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    path = out / f"{args.source}.jsonl"
    done = set()
    if path.exists():
        for line in path.read_text(encoding="utf-8").splitlines():
            rec = json.loads(line)
            if rec["judge_nodes"] != args.nodes:
                sys.exit(f"{path} was judged at {rec['judge_nodes']} nodes, not {args.nodes}")
            done.add(rec["index"])
    records = [game_record(args.source, i, g)
               for i, g in read_games(pathlib.Path(args.pgn), args.only, args.first)]
    todo = [r for r in records if r["index"] not in done]
    print(f"{args.source}: {len(records)} games selected, {len(done)} already judged, "
          f"{len(todo)} to judge at {args.nodes:,} nodes on {args.workers} workers", flush=True)
    cpus = [int(c) for c in args.cpus.split(",")] if args.cpus else []
    manager = multiprocessing.Manager()
    shared = manager.list(cpus)
    lock = manager.Lock()
    started = time.monotonic()
    with path.open("a", encoding="utf-8") as fh, futures.ProcessPoolExecutor(
            args.workers, initializer=_init_worker, initargs=(args.judge, shared, lock)) as pool:
        pending = [pool.submit(_judge_game, r, args.nodes) for r in todo]
        for n, fut in enumerate(futures.as_completed(pending), 1):
            fh.write(json.dumps(fut.result()) + "\n")
            fh.flush()
            if n % 50 == 0 or n == len(pending):
                rate = n / max(time.monotonic() - started, 1e-9) * 3600
                print(f"  {n}/{len(pending)} games, {rate:.0f} an hour", flush=True)
    return 0


# ---- Tables ------------------------------------------------------------------

def moves_of(record: dict, ours: re.Pattern) -> list[dict]:
    """Every judged move of a game with its side, cell and loss."""
    out = []
    white = record["first_white"]
    for k, uci in enumerate(record["moves"]):
        player = record["white"] if white else record["black"]
        before, after = record["judge"][k], record["judge"][k + 1]
        raw = move_loss(before, after)
        base = None if record["base_ms"] is None else record["base_ms"][0 if white else 1]
        out.append({"ply": k, "move": uci, "player": player,
                    "side": "ours" if ours.search(player) else "opponent",
                    "class": position_class(before),
                    "clock": clock_band(record["clock_ms"][k], base),
                    "material": material_band(record["men"][k]),
                    "raw_loss": raw, "loss": capped(raw)})
        white = not white
    return out


def tabulate(moves: list[dict]) -> dict:
    cells: dict = {}
    totals: dict = {}
    for m in moves:
        for key in ((m["side"], "all", "all", "all"),
                    (m["side"], m["class"], "all", "all"),
                    (m["side"], m["class"], m["clock"], "all"),
                    (m["side"], m["class"], "all", m["material"])):
            c = cells.setdefault(" | ".join(key), {"moves": 0, "loss": 0, "errors": 0})
            c["moves"] += 1
            c["loss"] += m["loss"]
            c["errors"] += m["loss"] >= ERROR
        totals[m["side"]] = totals.get(m["side"], 0) + m["loss"]
    for key, c in cells.items():
        side = key.split(" | ")[0]
        c["mean"] = c["loss"] / c["moves"]
        c["error_share"] = c["errors"] / c["moves"]
        c["loss_share"] = c["loss"] / totals[side] if totals[side] else 0.0
    return cells


def excess(cells: dict) -> list[tuple[str, float]]:
    """Our loss in each cell beyond the opponents' rate in the same cell
    (our moves times the difference of the means), largest first; only the
    cells both sides have."""
    out = []
    for key, c in cells.items():
        side, rest = key.split(" | ", 1)
        if side != "ours" or rest == "all | all | all":
            continue
        other = cells.get(f"opponent | {rest}")
        if other:
            out.append((rest, c["moves"] * (c["mean"] - other["mean"])))
    return sorted(out, key=lambda kv: -kv[1])


def run_tables(args) -> int:
    out = pathlib.Path(args.out)
    ours = re.compile(args.ours)
    report = {}
    for path in sorted(out.glob("*.jsonl")):
        if path.name.endswith(".research.jsonl"):
            continue
        moves = []
        games = 0
        for line in path.read_text(encoding="utf-8").splitlines():
            moves += moves_of(json.loads(line), ours)
            games += 1
        cells = tabulate(moves)
        ranked = excess(cells)
        report[path.stem] = {"games": games, "moves": len(moves), "cells": cells,
                             "excess": ranked}
        print(f"== {path.stem}: {games} games, {len(moves)} moves")
        print(f"{'side | class | clock | material':52} {'moves':>8} {'mean cp':>8} "
              f"{'>=50 cp':>8} {'of side loss':>12}")
        for key in sorted(cells):
            c = cells[key]
            print(f"{key:52} {c['moves']:>8,} {c['mean']:>8.2f} {100 * c['error_share']:>7.2f}% "
                  f"{100 * c['loss_share']:>11.1f}%")
        if ranked:
            print("our excess over the opponents' rate, cp (largest first):")
            for rest, cp in ranked[:8]:
                print(f"  {rest:48} {cp:>10,.0f}")
    (out / "tables.json").write_text(json.dumps(report, indent=1), encoding="utf-8")
    return 0


# ---- Re-search of the worst errors -------------------------------------------

LOST = -300   # a position the judge scores at or below this is already lost


def is_open(before: dict) -> bool:
    """Not already lost before the move: an error there costs the game."""
    return value(before) > LOST


def error_rank(e: dict) -> tuple:
    """The worst errors first: by counted loss, then from the better position
    (an error from a level position costs more than one from a bad one)."""
    return (-capped(e["game_raw_loss"]), -value(e["before"]), e["index"], e["ply"])

def _research_one(task: dict, engines: dict[str, str], judge_nodes: int,
                  factors: list[int]) -> dict:
    # Every score here comes from a cleared table, the game's move included,
    # so the moves compared are judged alike (the game pass judged each
    # position after the game's earlier ones).
    judge: Uci = _worker["judge"]

    def fresh(moves: list[str]) -> dict:
        board = chess.Board(task["fen"])
        for m in moves:
            board.push_uci(m)
        fixed = terminal_score(board)
        if fixed is not None:
            return fixed
        judge.new_game()
        return judge.search(task["fen"], moves, judge_nodes)[0]

    before = fresh(task["moves"])
    task["fresh_before"] = before
    task["fresh_game_loss"] = move_loss(before, fresh(task["moves"] + [task["game_move"]]))
    played = Uci(engines[task["player"]], {"Threads": "1", "Hash": "64"}, _worker["cpu"])
    try:
        for factor in factors:
            played.new_game()
            _, best = played.search(task["fen"], task["moves"], task["game_nodes"] * factor)
            task[f"x{factor}"] = {"move": best,
                                  "raw_loss": move_loss(before, fresh(task["moves"] + [best]))}
    finally:
        played.close()
    return task


def run_research(args) -> int:
    out = pathlib.Path(args.out)
    ours = re.compile(args.ours)
    engines = {name: str(pathlib.Path(path).resolve())
               for name, path in (e.split("=", 1) for e in args.engine)}
    errors = []
    for line in (out / f"{args.source}.jsonl").read_text(encoding="utf-8").splitlines():
        rec = json.loads(line)
        for m in moves_of(rec, ours):
            meta = rec["meta"][m["ply"]]
            if (m["side"] != "ours" or m["loss"] < args.min_loss or not meta
                    or not is_open(rec["judge"][m["ply"]])):
                continue
            errors.append({"index": rec["index"], "ply": m["ply"], "player": m["player"],
                           "game_move": m["move"], "game_raw_loss": m["raw_loss"],
                           "class": m["class"], "clock": m["clock"], "material": m["material"],
                           "fen": rec["fen"], "moves": rec["moves"][:m["ply"]],
                           "before": rec["judge"][m["ply"]], "game_nodes": meta["n"]})
    errors.sort(key=error_rank)
    chosen = errors[: args.top]
    missing = {e["player"] for e in chosen} - set(engines)
    if missing:
        sys.exit(f"no binary given for {sorted(missing)}")
    print(f"{args.source}: {len(errors)} errors of {args.min_loss}+ cp, re-searching "
          f"{len(chosen)} at x{','.join(map(str, args.factors))}", flush=True)
    cpus = [int(c) for c in args.cpus.split(",")] if args.cpus else []
    manager = multiprocessing.Manager()
    shared, lock = manager.list(cpus), manager.Lock()
    results = []
    with futures.ProcessPoolExecutor(args.workers, initializer=_init_worker,
                                     initargs=(args.judge, shared, lock)) as pool:
        pending = [pool.submit(_research_one, e, engines, args.nodes, args.factors)
                   for e in chosen]
        for fut in futures.as_completed(pending):
            results.append(fut.result())
    results.sort(key=error_rank)
    path = out / f"{args.source}.research.jsonl"
    path.write_text("".join(json.dumps(r) + "\n" for r in results), encoding="utf-8")
    summary = summarise_research(results, args.factors)
    (out / f"{args.source}.research.json").write_text(json.dumps(summary, indent=1),
                                                      encoding="utf-8")
    print(json.dumps(summary, indent=1))
    return 0


def summarise_research(results: list[dict], factors: list[int]) -> dict:
    """How many errors the game's own search repeats at 1x, and how many the
    deeper searches mend (their move loses under ERROR cp)."""
    out = {"errors": len(results),
           "still_errors_rejudged": sum(r.get("fresh_game_loss", ERROR) >= ERROR
                                        for r in results)}
    first = factors[0]
    out[f"repeated_at_x{first}"] = sum(r[f"x{first}"]["move"] == r["game_move"]
                                       for r in results)
    for f in factors:
        out[f"mended_at_x{f}"] = sum(r[f"x{f}"]["raw_loss"] < ERROR for r in results)
    deepest = factors[-1]
    out["persist_at_deepest"] = sum(r[f"x{deepest}"]["raw_loss"] >= ERROR for r in results)
    return out


# ---- Main --------------------------------------------------------------------

def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = ap.add_subparsers(dest="mode", required=True)
    j = sub.add_parser("judge")
    j.add_argument("--pgn", required=True)
    j.add_argument("--source", required=True)
    j.add_argument("--only", default=None, help="a player regex the game must have")
    j.add_argument("--first", type=int, default=0, help="the first N selected games")
    j.add_argument("--judge", required=True)
    j.add_argument("--nodes", type=int, default=150_000)
    j.add_argument("--workers", type=int, default=1)
    j.add_argument("--cpus", default="", help="comma-separated logical CPUs, one per worker")
    j.add_argument("--out", required=True)
    t = sub.add_parser("tables")
    t.add_argument("--out", required=True)
    t.add_argument("--ours", default=r"^[Bb]asilisk")
    r = sub.add_parser("research")
    r.add_argument("--out", required=True)
    r.add_argument("--source", required=True)
    r.add_argument("--ours", default=r"^[Bb]asilisk")
    r.add_argument("--engine", action="append", default=[], metavar="PLAYER=PATH")
    r.add_argument("--judge", required=True)
    r.add_argument("--nodes", type=int, default=150_000)
    r.add_argument("--top", type=int, default=100)
    r.add_argument("--min-loss", type=int, default=100)
    r.add_argument("--factors", type=lambda s: [int(x) for x in s.split(",")], default=[1, 4, 16])
    r.add_argument("--workers", type=int, default=1)
    r.add_argument("--cpus", default="")
    args = ap.parse_args()
    if args.mode in ("judge", "research"):
        args.judge = str(pathlib.Path(args.judge).resolve())
    return {"judge": run_judge, "tables": run_tables, "research": run_research}[args.mode](args)


if __name__ == "__main__":
    sys.exit(main())
