#!/usr/bin/env python3
"""Nodes per search under the gate's real clock, and the budget bracket it sets.

Every fixed-node screen claims that its budget stands in for what the engine
really searches in a gate. This measures that claim: games at the gate's
conditions, the engine's own time manager reading genuine `wtime/btime/winc/
binc` (a fixed `movetime` would run another code path), and `nodes` taken from
each search's final info.

Conditions follow the Colosseum gate (`tools/colosseum/common.toml`): starts
from the UHO book, two engine processes per game (one per side, so neither
side reads the other's table), one thread, Hash 64, and `--concurrency` games
at once so the host carries the gate's load: a lone game on an idle host runs
at a higher clock than fifteen do. The host must otherwise be idle.

The clock is charged with MEASURED wall time and credited the increment after
each move, so an engine that overspends is not given the time back; a game
whose clock runs out is counted as a forfeit, not hidden.

Splits are descriptive: by ply (the tree narrows as the game simplifies) and
by men on the board. The endgame truth cohort is three to six men, so the
bracket is read from the moves made with at most six men on the board when
there are at least MIN_ENDGAME_MOVES of them, and from all moves otherwise
(`bracket_from`; registered in BAS-D23 before the first read).

This is a run-condition layer, not a measurement of play.

Example (the gate's conditions on a 16-core host):

  python tools/diag/nodes_per_move.py --engine <release.exe> --games 150 \\
      --concurrency 15 --output tools/results/c273/nodes_per_move.json
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import random
import statistics
import sys
import threading
import time
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import chess
import chess.engine

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_BOOK = ROOT / "tools" / "books" / "UHO_Lichess_4852_v1.epd"
ENDGAME_MEN = 6
MIN_ENDGAME_MOVES = 300
PLY_BANDS = [("opening", 0, 30), ("middlegame", 30, 80), ("endgame", 80, 10**9)]
MEN_BANDS = [("men_3_6", 3, 6), ("men_7_12", 7, 12), ("men_13_32", 13, 32)]


def parse_tc(spec: str) -> tuple[float, float]:
    """`3+0.03` -> (3.0, 0.03) seconds."""
    if "+" not in spec:
        raise ValueError(f"time control must look like 3+0.03, got {spec!r}")
    base, inc = spec.split("+", 1)
    base_s, inc_s = float(base), float(inc)
    if base_s <= 0 or inc_s < 0:
        raise ValueError(f"time control must be positive, got {spec!r}")
    return base_s, inc_s


def sample_book(path: Path, count: int, seed: int) -> list[str]:
    """`count` book positions at seeded random byte offsets.

    Each pick seeks to an offset and takes the next whole line, so the book
    (millions of lines) is never read into memory. A line's chance is
    proportional to the length of the line before it, which for FENs of
    similar length is a negligible bias.
    """
    rng = random.Random(seed)
    size = path.stat().st_size
    out = []
    with path.open("rb") as book:
        while len(out) < count:
            book.seek(rng.randrange(size))
            book.readline()
            line = book.readline().decode("ascii").strip()
            if not line:
                continue
            fen = " ".join(line.split()[:6]) if line.count(" ") >= 5 else line.split(";")[0].strip()
            board = chess.Board(fen)
            if board.is_valid() and not board.is_game_over():
                out.append(board.fen())
    return out


def quantiles(values: list[int]) -> dict:
    ordered = sorted(values)
    n = len(ordered)

    def at(q: float) -> int:
        return ordered[min(n - 1, max(0, int(round(q * (n - 1)))))]

    return {
        "n": n,
        "min": ordered[0],
        "p25": at(0.25),
        "median": int(statistics.median(ordered)),
        "p75": at(0.75),
        "p90": at(0.90),
        "max": ordered[-1],
        "mean": int(statistics.fmean(ordered)),
    }


def round_sig(value: float, digits: int = 2) -> int:
    """Round to `digits` significant figures: 137,412 -> 140,000."""
    if value <= 0:
        raise ValueError("a budget must be positive")
    magnitude = len(str(int(value))) - digits
    step = 10 ** max(0, magnitude)
    return int(round(value / step) * step)


def bracket_from(samples: list[dict]) -> dict:
    """The registered rule: three budgets M/3, M and 3M, two significant figures.

    M is the median nodes per search over moves made with at most ENDGAME_MEN
    men on the board when there are at least MIN_ENDGAME_MOVES of them, and the
    median over every move otherwise.
    """
    endgame = [s["nodes"] for s in samples if s["men"] <= ENDGAME_MEN]
    if len(endgame) >= MIN_ENDGAME_MOVES:
        source, values = f"moves with at most {ENDGAME_MEN} men", endgame
    else:
        source, values = "every move (too few endgame moves)", [s["nodes"] for s in samples]
    median = statistics.median(values)
    return {
        "source": source,
        "moves": len(values),
        "median": int(median),
        "budgets": [round_sig(median / 3), round_sig(median), round_sig(median * 3)],
    }


def open_engine(path: Path, hash_mb: int) -> chess.engine.SimpleEngine:
    engine = chess.engine.SimpleEngine.popen_uci(str(path))
    options = {}
    if "Hash" in engine.options:
        options["Hash"] = hash_mb
    if "Threads" in engine.options:
        options["Threads"] = 1
    if "SyzygyPath" in engine.options:
        options["SyzygyPath"] = ""  # the gate plays without tables
    if options:
        engine.configure(options)
    return engine


def play_game(engines, fen: str, base: float, inc: float, max_plies: int) -> dict:
    """One game from `fen`, engines[0] White. Returns its samples and ending."""
    board = chess.Board(fen)
    clock = {chess.WHITE: base, chess.BLACK: base}
    token = object()
    samples = []
    ending = "max_plies"
    for ply in range(max_plies):
        if board.is_game_over(claim_draw=True):
            ending = "game_over"
            break
        side = board.turn
        engine = engines[0] if side == chess.WHITE else engines[1]
        limit = chess.engine.Limit(white_clock=clock[chess.WHITE], black_clock=clock[chess.BLACK],
                                   white_inc=inc, black_inc=inc)
        started = time.perf_counter()
        result = engine.play(board, limit, info=chess.engine.INFO_BASIC, game=token)
        elapsed = time.perf_counter() - started
        if result.move is None:
            ending = "no_move"
            break
        nodes = result.info.get("nodes")
        if nodes is not None:
            samples.append({"ply": ply, "men": chess.popcount(board.occupied),
                            "nodes": int(nodes), "seconds": round(elapsed, 4)})
        board.push(result.move)
        clock[side] = clock[side] - elapsed + inc
        if clock[side] <= 0:
            ending = "forfeit"
            break
    return {"samples": samples, "ending": ending, "plies": len(board.move_stack)}


class Slot:
    """One concurrent game slot: its own two engines, reused game after game."""

    def __init__(self, path: Path, hash_mb: int) -> None:
        self.engines = [open_engine(path, hash_mb), open_engine(path, hash_mb)]

    def close(self) -> None:
        for engine in self.engines:
            try:
                engine.quit()
            except Exception:
                pass


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--engine", required=True, type=Path)
    ap.add_argument("--tc", default="3+0.03", help="base+increment in seconds")
    ap.add_argument("--games", type=int, default=150)
    ap.add_argument("--concurrency", type=int, default=15)
    ap.add_argument("--max-plies", type=int, default=400)
    ap.add_argument("--hash", type=int, default=64)
    ap.add_argument("--book", type=Path, default=DEFAULT_BOOK)
    ap.add_argument("--seed", type=int, default=0xC273)
    ap.add_argument("--output", type=Path)
    args = ap.parse_args()

    engine_path = args.engine.resolve()
    if not engine_path.is_file():
        ap.error(f"engine not found: {engine_path}")
    if not args.book.is_file():
        ap.error(f"book not found: {args.book}")
    try:
        base, inc = parse_tc(args.tc)
    except ValueError as exc:
        ap.error(str(exc))
    if args.games < 1 or args.concurrency < 1:
        ap.error("games and concurrency must be at least 1")

    starts = sample_book(args.book, args.games, args.seed)
    slots_free: list[Slot] = []
    lock = threading.Lock()
    all_slots: list[Slot] = []

    def run(index: int) -> dict:
        with lock:
            slot = slots_free.pop() if slots_free else None
        if slot is None:
            slot = Slot(engine_path, args.hash)
            with lock:
                all_slots.append(slot)
        try:
            game = play_game(slot.engines, starts[index], base, inc, args.max_plies)
        finally:
            with lock:
                slots_free.append(slot)
        print(f"game {index + 1}/{args.games}: {len(game['samples'])} moves, {game['ending']}",
              flush=True)
        return game

    try:
        with ThreadPoolExecutor(max_workers=min(args.concurrency, args.games)) as pool:
            games = list(pool.map(run, range(args.games)))
    finally:
        for slot in all_slots:
            slot.close()

    samples = [s for g in games for s in g["samples"]]
    if not samples:
        print("no samples: the engine reported no node counts", file=sys.stderr)
        return 1
    endings: dict[str, int] = {}
    for g in games:
        endings[g["ending"]] = endings.get(g["ending"], 0) + 1

    report = {
        "schema": "basilisk-nodes-per-move-v1",
        "layer": "run_condition",
        "layer_note": "not a measurement of play: it sizes the node budgets a "
                      "fixed-node screen must justify itself against",
        "engine": str(engine_path),
        "engine_sha256": sha256_file(engine_path),
        "time_control": args.tc,
        "games": args.games,
        "concurrency": args.concurrency,
        "engine_processes_per_game": 2,
        "threads": 1,
        "hash_mb": args.hash,
        "book": str(args.book.resolve()),
        "book_seed": args.seed,
        "starts_sha256": hashlib.sha256("\n".join(starts).encode("ascii")).hexdigest(),
        "host": {"platform": platform.platform(), "machine": platform.machine(),
                 "logical_cpus": os.cpu_count()},
        "endings": endings,
        "overall": quantiles([s["nodes"] for s in samples]),
        "by_ply": {},
        "by_men": {},
        "bracket": bracket_from(samples),
    }
    for name, low, high in PLY_BANDS:
        band = [s["nodes"] for s in samples if low <= s["ply"] < high]
        if band:
            report["by_ply"][name] = quantiles(band)
    for name, low, high in MEN_BANDS:
        band = [s["nodes"] for s in samples if low <= s["men"] <= high]
        if band:
            report["by_men"][name] = quantiles(band)

    rendered = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(rendered, encoding="utf-8", newline="\n")
        print(f"Report: {args.output.resolve()}")
    o = report["overall"]
    print(f"\nnodes per search at {args.tc}, {o['n']} moves over {args.games} games "
          f"({args.concurrency} at once): median {o['median']:,}, p25 {o['p25']:,}, "
          f"p75 {o['p75']:,}")
    for name, q in report["by_men"].items():
        print(f"  {name:<10} median {q['median']:,} over {q['n']} moves")
    print(f"  endings {endings}")
    b = report["bracket"]
    print(f"bracket from {b['source']} (median {b['median']:,} over {b['moves']} moves): "
          + ", ".join(f"{x:,}" for x in b["budgets"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
