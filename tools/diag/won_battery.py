#!/usr/bin/env python3
"""The battery screen: what a build spends on positions it has in hand.

A clocked read, zero-game. Positions come from one Colosseum PGN, games in file
order, one position a game and stratum, judged by the mover's own reported score:

  R  decided endings the review read, kept out of E: the first such position of
     the first `--skip` games that have one (a calibration, read only by name)
  E  decided endings: 1,500 to 29,999 with at most eight men, the next ones after R
  W  better and not decided: 300 to 1,499 on this move and on the mover's previous
     one, any material
  six  the packet's six won endings, in this file

A fresh process a search, Threads 1, Hash 64, `TM_Debug` and `Diag` on, options by
`--option`. `Diag` is there for one counter: the hole is a root search that ends
late *without the stop firing*, and only `fhstop_fired` says whether it fired.

Per column and stratum: searches over 0.5x the maximum and at it (0.95x or more),
spend, searches past the optimum, root best-move changes per completed iteration,
and the holes. The engine reports its longest root search, so a hole is that search
ending after twice the optimum, or at the maximum, split by whether it began before
the optimum. A search that ends late while its longest ended earlier is named
"other" and is not split: nothing says where its last root search began.

An engine that never answers, or dies, is counted and does not hang the read; the
read then exits 1 and keeps its rows. Exit 2 is bad input, nothing searched.

Usage:
  python tools/diag/won_battery.py positions --pgn games.pgn --out positions.json
  python tools/diag/won_battery.py read --engine tune.exe --positions positions.json \
      --clock 60000+600 --strata E,W,six --label arm-1 --out arm-1.json [--option B5FailHighStop=0]
  python tools/diag/won_battery.py report arm-1.json arm-2.json off.json
  python tools/diag/won_battery.py gate --base arm-1.json --base arm-2.json --option-column stop0.json
"""

from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import json
import pathlib
import re
import subprocess
import sys
import threading
import time

try:
    import chess
    import chess.pgn
except ImportError:  # pragma: no cover
    sys.exit("python-chess is required: pip install chess")

# The direction run's PGN and the review's skip, count and threshold (packet §A2.5).
REGISTERED_PGN_SHA256 = "6931997c5ee5b5cf476f7ca8623e74f996f55c7fb791ac26d9b863da4021725e"
SKIP, DECIDED, BETTER = 60, 120, 60
DECIDED_MIN, MATE_SCORE, MAX_MEN = 1500, 30000, 8
BETTER_MIN, BETTER_MAX = 300, 1499
OVER_SHARE, AT_MAX_SHARE = 0.5, 0.95
GATE_COUNT, GATE_SPEND_TENTHS = 2, 11

SIX = (
    {"name": "rec1", "fen": "7r/5R2/8/2k1PB2/8/4K3/8/8 w - - 0 86"},
    {"name": "rec2", "fen": "8/P4k2/8/1N6/1P2B1K1/8/8/8 w - - 7 81"},
    {"name": "rec3", "fen": "1r6/R7/6k1/8/8/5PP1/6K1/8 w - - 6 72"},
    {"name": "krbk", "fen": "8/8/8/4k3/8/8/2B5/R3K3 w - - 0 70"},
    {"name": "krk", "fen": "8/8/3k4/8/8/8/8/R3K3 w - - 0 70"},
    {"name": "kbnk", "fen": "8/8/8/3k4/8/8/8/1NB1K3 w - - 0 70"},
)

SCORE_COMMENT = re.compile(r"s=(-?\d+) ")
EXACT_DEPTH = re.compile(r"^info depth (\d+)\b")
SCORE = re.compile(r"\bscore (\w+ -?\d+)")
TM_LINE = re.compile(r"soft_ms=(-?\d+) hard_ms=(-?\d+) elapsed_ms=(-?\d+) dispatch_ms=(-?\d+)"
                     r" longest_window_start_ms=(-?\d+) longest_window_ms=(-?\d+)")
COUNTER = re.compile(r"\b(fhstop_fired|interior_nodes|qs_nodes)=(-?\d+)")


def sha256(path: pathlib.Path) -> str:
    return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()


def sample(handle, skip: int = SKIP, decided: int = DECIDED, better: int = BETTER) -> dict[str, list[dict]]:
    """The strata of a PGN: deterministic, one position a game and stratum."""
    review: list[dict] = []
    ending: list[dict] = []
    ahead: list[dict] = []
    while len(review) < skip or len(ending) < decided or len(ahead) < better:
        game = chess.pgn.read_game(handle)
        if game is None:
            break
        number = game.headers.get("GameNumber")
        board = game.board()
        previous = {chess.WHITE: None, chess.BLACK: None}
        took_decided = took_better = False
        for node in game.mainline():
            match = SCORE_COMMENT.search(node.comment or "")
            score = int(match.group(1)) if match else None
            men = chess.popcount(board.occupied)
            if not took_decided and score is not None and DECIDED_MIN <= score < MATE_SCORE and men <= MAX_MEN:
                took_decided = True
                row = {"game": number, "fen": board.fen(), "s": score, "men": men}
                if len(review) < skip:
                    review.append(row)
                elif len(ending) < decided:
                    ending.append(row)
            before = previous[board.turn]
            if (not took_better and score is not None and BETTER_MIN <= score <= BETTER_MAX
                    and before is not None and BETTER_MIN <= before <= BETTER_MAX):
                took_better = True
                if len(ahead) < better:
                    ahead.append({"game": number, "fen": board.fen(), "s": score, "previous": before, "men": men})
            previous[board.turn] = score
            board.push(node.move)
    return {"R": review, "E": ending, "W": ahead}


def hole_class(soft: int, hard: int, start: int, length: int, wall: float, fired: int | None) -> str | None:
    """Where a root search that ended late, with the stop silent, began."""
    if fired:
        return None
    end = start + length
    if end > 2 * soft or end >= AT_MAX_SHARE * hard:
        return "before" if start < soft else "after"
    if wall > 2 * soft or wall >= AT_MAX_SHARE * hard:
        return "other"
    return None


def parse_search(lines: list[str], wall_ms: float) -> dict:
    """One search's row from its output."""
    completed: dict[int, str] = {}
    for line in lines:
        match = EXACT_DEPTH.match(line)
        if match and " score " in line and "bound" not in line and " pv " in line:
            completed[int(match.group(1))] = line
    firsts = [completed[depth].split(" pv ")[1].split()[0] for depth in completed]
    last = completed[next(reversed(completed))] if completed else ""
    tm = next((m for m in map(TM_LINE.search, lines) if m), None)
    counters = {name: int(value) for line in lines for name, value in COUNTER.findall(line)}
    best = next((line.split() for line in lines if line.startswith("bestmove")), None)
    nodes_last = re.search(r" nodes (\d+)", last)
    score = SCORE.search(last)
    row = {"elapsed_ms": round(wall_ms), "soft_ms": None, "hard_ms": None, "share": None,
           "depth": max(completed, default=None), "score": score.group(1) if score else None,
           "nodes": counters["interior_nodes"] + counters["qs_nodes"] if {"interior_nodes", "qs_nodes"} <= set(counters) else None,
           "nodes_last_depth": int(nodes_last.group(1)) if nodes_last else None,
           "iterations": len(firsts), "changes": sum(a != b for a, b in zip(firsts, firsts[1:])),
           "fired": counters.get("fhstop_fired"), "longest_start_ms": None, "longest_ms": None, "hole": None,
           "bestmove": best[1] if best and len(best) > 1 else None}
    if tm:
        soft, hard, _, _, start, length = map(int, tm.groups())
        row.update(soft_ms=soft, hard_ms=hard, longest_start_ms=start, longest_ms=length,
                   share=round(wall_ms / hard, 3) if hard else None,
                   hole=hole_class(soft, hard, start, length, wall_ms, row["fired"]))
    return row


class Process:
    """An engine whose output is read on its own thread, so a silent one cannot hang a wait."""

    def __init__(self, command: list[str]) -> None:
        self.proc = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                                     text=True, bufsize=1, errors="replace")
        self.lines: list[str] = []
        self.ended = False
        self.best_at: float | None = None
        self.changed = threading.Condition()
        self.thread = threading.Thread(target=self.read, daemon=True)
        self.thread.start()

    def read(self) -> None:
        for line in self.proc.stdout:
            with self.changed:
                if line.startswith("bestmove") and self.best_at is None:
                    self.best_at = time.perf_counter()
                self.lines.append(line.rstrip())
                self.changed.notify_all()
        with self.changed:
            self.ended = True
            self.changed.notify_all()

    def send(self, text: str) -> bool:
        try:
            self.proc.stdin.write(text)
            self.proc.stdin.flush()
            return True
        except OSError:
            return False

    def wait_for(self, prefix: str, seconds: float, since: int = 0) -> bool:
        deadline = time.monotonic() + seconds
        with self.changed:
            while True:
                if any(line.startswith(prefix) for line in self.lines[since:]):
                    return True
                left = deadline - time.monotonic()
                if self.ended or left <= 0:
                    return False
                self.changed.wait(min(left, 0.25))

    def finish(self) -> int | None:
        """Ends it however it stands; the exit status when it ended by itself on `quit`."""
        code = None
        if self.send("quit\n"):
            try:
                code = self.proc.wait(timeout=10)
            except subprocess.TimeoutExpired:
                pass
        if self.proc.poll() is None:
            self.proc.kill()
            self.proc.wait()
        elif code is None:
            code = self.proc.returncode
        self.thread.join(5)
        for stream in (self.proc.stdin, self.proc.stdout):
            try:
                stream.close()
            except OSError:
                pass  # its other end has gone
        return code


def search(command: list[str], fen: str, wtime: int, inc: int, options: list[str], hash_mb: int, watchdog: float) -> dict:
    """One position in a fresh process. Its clock starts once the engine is ready."""
    engine = Process(command)
    script = (f"setoption name Threads value 1\nsetoption name Hash value {hash_mb}\n"
              "setoption name TM_Debug value true\nsetoption name Diag value true\n")
    for option in options:
        name, value = option.split("=", 1)
        script += f"setoption name {name} value {value}\n"
    script += f"ucinewgame\nposition fen {fen}\nisready\n"
    answered = False
    began = None
    if engine.send(script) and engine.wait_for("readyok", watchdog):
        mark = len(engine.lines)
        began = time.perf_counter()
        if engine.send(f"go wtime {wtime} btime {wtime} winc {inc} binc {inc}\n"):
            answered = engine.wait_for("bestmove", watchdog, mark)
    if answered:
        # The diagnostic and tm lines can follow bestmove.
        mark = len(engine.lines)
        if engine.send("isready\n"):
            engine.wait_for("readyok", 30.0, mark)
    code = engine.finish()
    wall = (engine.best_at - began) * 1000.0 if answered and began is not None and engine.best_at else 0.0
    row = parse_search(engine.lines, wall)
    row.update(answered=answered, exit=code)
    if not answered:
        row.update(elapsed_ms=None, share=None, hole=None, bestmove=None)
    return row


def summarize(rows: list[dict]) -> dict[str, dict]:
    """The columns of each stratum."""
    table: dict[str, dict] = {}
    for stratum in dict.fromkeys(row["stratum"] for row in rows):
        here = [row for row in rows if row["stratum"] == stratum]
        good = [row for row in here if row["answered"]]
        iterations = sum(row["iterations"] for row in good)
        table[stratum] = {
            "searches": len(here),
            "unanswered": len(here) - len(good),
            "over_half": sum(row["elapsed_ms"] > OVER_SHARE * row["hard_ms"] for row in good),
            "at_max": sum(row["elapsed_ms"] >= AT_MAX_SHARE * row["hard_ms"] for row in good),
            "spend_ms": sum(row["elapsed_ms"] for row in good),
            "past_optimum": sum(row["elapsed_ms"] > row["soft_ms"] for row in good),
            "changes_per_iteration": round(sum(row["changes"] for row in good) / iterations, 3) if iterations else None,
            "hole_before": sum(row["hole"] == "before" for row in good),
            "hole_after": sum(row["hole"] == "after" for row in good),
            "hole_other": sum(row["hole"] == "other" for row in good),
            "fired": sum(bool(row["fired"]) for row in good),
        }
    return table


def differential_gate(first: dict, second: dict, option: dict) -> dict:
    """§A2.5's gate: the option against the two runs at 1x of the same binary, per stratum."""
    verdict = {"passes": True, "strata": {}}
    for stratum in first:
        if stratum not in option or stratum not in second:
            verdict["strata"][stratum] = {"missing": True}
            verdict["passes"] = False
            continue
        count_limit = max(first[stratum]["over_half"], second[stratum]["over_half"]) + GATE_COUNT
        spend_base = max(first[stratum]["spend_ms"], second[stratum]["spend_ms"])
        count_ok = option[stratum]["over_half"] <= count_limit
        spend_ok = option[stratum]["spend_ms"] * 10 <= spend_base * GATE_SPEND_TENTHS
        verdict["strata"][stratum] = {"over_half": option[stratum]["over_half"], "count_limit": count_limit, "count_ok": count_ok,
                                      "spend_ms": option[stratum]["spend_ms"], "spend_base_ms": spend_base, "spend_ok": spend_ok}
        verdict["passes"] = verdict["passes"] and count_ok and spend_ok
    return verdict


def advertised(command: list[str], names: list[str]) -> list[str]:
    """The names the engine does not advertise; every name when it will not say."""
    engine = Process(command)
    told = engine.send("uci\n") and engine.wait_for("uciok", 30.0)
    text = "\n".join(engine.lines).lower()
    engine.finish()
    if not told:
        return names
    return [name for name in names if f"option name {name.lower()} " not in text]


def command_positions(args: argparse.Namespace) -> int:
    digest = sha256(args.pgn)
    if digest != REGISTERED_PGN_SHA256:
        print(f"{args.pgn} is {digest}, not the registered PGN", file=sys.stderr)
        return 2
    with args.pgn.open(encoding="utf-8", errors="replace") as handle:
        strata = sample(handle, args.skip, args.decided, args.better)
    wanted = {"R": args.skip, "E": args.decided, "W": args.better}
    short = [f"{name} {len(strata[name])} of {count}" for name, count in wanted.items() if len(strata[name]) < count]
    if short:
        print(f"the PGN runs out: {', '.join(short)}", file=sys.stderr)
        return 2
    args.out.write_text(json.dumps({"pgn": str(args.pgn), "pgn_sha256": digest, "strata": strata}, indent=1), encoding="utf-8")
    print(f"{args.out}: R {len(strata['R'])}, E {len(strata['E'])}, W {len(strata['W'])}; sha256 {sha256(args.out)}")
    return 0


def command_read(args: argparse.Namespace) -> int:
    if args.command_json:
        command = json.loads(args.command_json.read_text(encoding="utf-8"))
        identity = None
    elif args.engine and args.engine.exists():
        command = [str(args.engine.resolve())]
        identity = sha256(args.engine)
    else:
        print(f"no engine at {args.engine}", file=sys.stderr)
        return 2
    match = re.fullmatch(r"(\d+)\+(\d+)", args.clock)
    if not match or any("=" not in option for option in args.option):
        print("--clock takes WTIME+INC in ms, --option NAME=VALUE", file=sys.stderr)
        return 2
    wtime, inc = int(match.group(1)), int(match.group(2))
    wanted = args.strata.split(",")
    document = json.loads(args.positions.read_text(encoding="utf-8")) if args.positions else {"strata": {}, "pgn_sha256": None}
    strata = {"six": list(SIX), **document["strata"]}
    unknown = [name for name in wanted if not strata.get(name)]
    if unknown:
        print(f"no positions for {unknown}", file=sys.stderr)
        return 2
    missing = advertised(command, ["Diag", "TM_Debug"] + [option.split("=", 1)[0] for option in args.option])
    if missing:
        print(f"the engine does not advertise {missing}", file=sys.stderr)
        return 2
    watchdog = args.watchdog if args.watchdog else (wtime + 10000) / 1000.0

    def job(item: tuple[str, int, dict]) -> dict:
        stratum, index, position = item
        row = search(command, position["fen"], wtime, inc, args.option, args.hash, watchdog)
        return {"stratum": stratum, "index": index, "game": position.get("game", position.get("name")), "fen": position["fen"],
                "s": position.get("s"), **row}

    jobs = [(name, index, position) for name in wanted for index, position in enumerate(strata[name])]
    began = time.time()
    with concurrent.futures.ThreadPoolExecutor(args.workers) as pool:
        rows = list(pool.map(job, jobs))
    summary = summarize(rows)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps({
        "label": args.label, "engine": str(args.engine) if args.engine else command, "engine_sha256": identity,
        "options": args.option, "wtime": wtime, "inc": inc, "hash": args.hash, "threads": 1, "workers": args.workers,
        "watchdog_s": watchdog, "positions_pgn_sha256": document.get("pgn_sha256"), "strata": wanted,
        "wall_s": round(time.time() - began, 1), "summary": summary, "rows": rows}, indent=1), encoding="utf-8")
    print_table([(args.label, wtime, inc, summary)])
    crashed = [row for row in rows if row["answered"] and row["exit"] not in (0, None)]
    return 1 if crashed or any(row["unanswered"] for row in summary.values()) else 0


def print_table(columns: list[tuple[str, int, int, dict]]) -> None:
    print("column | clock | stratum | searches | unanswered | over 0.5x | at max | spend s | past optimum | changes/iter | "
          "hole before | hole after | other | stop fired")
    for label, wtime, inc, summary in columns:
        for stratum, row in summary.items():
            print(f"{label} | {wtime}+{inc} | {stratum} | {row['searches']} | {row['unanswered']} | {row['over_half']} | {row['at_max']} | "
                  f"{row['spend_ms'] / 1000:.1f} | {row['past_optimum']} | {row['changes_per_iteration']} | "
                  f"{row['hole_before']} | {row['hole_after']} | {row['hole_other']} | {row['fired']}", flush=True)


def load(path: pathlib.Path) -> dict:
    return json.loads(pathlib.Path(path).read_text(encoding="utf-8"))


def command_report(args: argparse.Namespace) -> int:
    documents = [load(path) for path in args.files]
    print_table([(d["label"], d["wtime"], d["inc"], d["summary"]) for d in documents])
    return 0


def command_gate(args: argparse.Namespace) -> int:
    first, second = (load(path) for path in args.base)
    option = load(args.option_column)
    identities = {first["engine_sha256"], second["engine_sha256"], option["engine_sha256"]}
    clocks = {(d["wtime"], d["inc"]) for d in (first, second, option)}
    if len(identities) != 1 or len(clocks) != 1:
        print(f"the gate reads one binary at one clock: {identities}, {clocks}", file=sys.stderr)
        return 2
    verdict = differential_gate(first["summary"], second["summary"], option["summary"])
    print(json.dumps({"option": option["label"], **verdict}, indent=1))
    return 0 if verdict["passes"] else 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="mode", required=True)
    positions = sub.add_parser("positions")
    positions.add_argument("--pgn", required=True, type=pathlib.Path)
    positions.add_argument("--out", required=True, type=pathlib.Path)
    positions.add_argument("--skip", type=int, default=SKIP)
    positions.add_argument("--decided", type=int, default=DECIDED)
    positions.add_argument("--better", type=int, default=BETTER)
    read = sub.add_parser("read")
    read.add_argument("--engine", type=pathlib.Path)
    read.add_argument("--command-json", type=pathlib.Path, help="a JSON command line, in place of --engine")
    read.add_argument("--positions", type=pathlib.Path)
    read.add_argument("--clock", required=True, help="WTIME+INC in ms")
    read.add_argument("--strata", required=True, help="comma-separated: R, E, W, six")
    read.add_argument("--option", action="append", default=[], metavar="NAME=VALUE")
    read.add_argument("--label", required=True)
    read.add_argument("--out", required=True, type=pathlib.Path)
    read.add_argument("--workers", type=int, default=8)
    read.add_argument("--hash", type=int, default=64)
    read.add_argument("--watchdog", type=float, help="seconds a search may take; default the clock and 10 s")
    report = sub.add_parser("report")
    report.add_argument("files", nargs="+", type=pathlib.Path)
    gate = sub.add_parser("gate")
    gate.add_argument("--base", required=True, action="append", type=pathlib.Path, help="the two runs at 1x")
    gate.add_argument("--option-column", required=True, type=pathlib.Path)
    args = parser.parse_args()
    if args.mode == "gate" and len(args.base) != 2:
        parser.error("--base twice: the two runs at 1x")
    return {"positions": command_positions, "read": command_read, "report": command_report, "gate": command_gate}[args.mode](args)


if __name__ == "__main__":
    sys.exit(main())
