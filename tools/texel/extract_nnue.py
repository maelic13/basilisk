#!/usr/bin/env python3
"""
Parallel PGN -> NNUE training-text extractor (Phase 9.1).

Emits one line per sampled position:

    <full FEN> | <score cp, white POV> | <result, white POV: 1.0 / 0.5 / 0.0>

which is the text shape bullet-utils converts to bulletformat. Unlike the
Texel extractors this keeps BOTH labels: the search score comes from the
fastchess move comments our datagen PGNs already carry ("{+0.28/6 0.030s}"),
the result from the game header. The blend (lambda) is chosen at training
time, so nothing is thrown away here.

Position filters (superset of extract.py's quiet filter):
  - skip the first --skip-start and last --skip-end plies
  - skip positions in check
  - skip positions where the PLAYED move is a capture or promotion
  - skip moves whose comment has no parsable score, is a mate score (M), or
    exceeds --max-cp (mate-adjacent adjudication noise)
  - sample at most --max-per-game positions per game (seeded by the game's
    movetext, so results are identical regardless of worker count)

Dedup is ON by default (64-bit hash of the FEN's first 4 fields, ~60 bytes
per unique position — budget ~4 GB at 60M uniques; --no-dedup to stream).

Usage:
    python tools/texel/extract_nnue.py <a.pgn> [b.pgn ...] \
        --out tools/texel/data/nnue_train.txt [--jobs N] [--max-per-game 24]
"""
from __future__ import annotations

import argparse
import hashlib
import io
import os
import random
import re
import sys
from multiprocessing import Pool

import chess.pgn

RESULT_MAP = {"1-0": 1.0, "0-1": 0.0, "1/2-1/2": 0.5}
# fastchess comment: "+0.28/6 0.030s" | "-1.05/5 ..." | "+M5/12 ..." | "0.00/6 ..."
SCORE_RE = re.compile(r"\s*([+-]?)(M?)(\d+(?:\.\d+)?)/")


def parse_score_cp(comment: str) -> int | None:
    """Mover-POV centipawns from a fastchess move comment; None if absent/mate."""
    m = SCORE_RE.match(comment)
    if not m:
        return None
    sign, mate, num = m.groups()
    if mate:
        return None
    cp = int(round(float(num) * 100))
    return -cp if sign == "-" else cp


def next_game_offset(path, nominal, filesize):
    """First byte offset of a game header ('[Event ') at or after `nominal`."""
    if nominal <= 0:
        return 0
    if nominal >= filesize:
        return filesize
    with open(path, "rb") as f:
        pos = max(0, nominal - 1)
        f.seek(pos)
        carry = b""
        base = pos
        while True:
            chunk = f.read(1 << 16)
            if not chunk:
                return filesize
            buf = carry + chunk
            idx = buf.find(b"\n[Event ")
            if idx != -1:
                return base - len(carry) + idx + 1
            carry = buf[-8:]
            base += len(chunk)


def process_game(game, opts):
    """Return a list of (fen, cp_white, result_white) rows for one game."""
    result = RESULT_MAP.get(game.headers.get("Result", "*"))
    if result is None:
        return []

    board = game.board()
    nodes = list(game.mainline())
    n = len(nodes)
    if n == 0:
        return []

    candidates = []
    for ply, node in enumerate(nodes):
        move = node.move
        keep = (
            opts["skip_start"] <= ply < n - opts["skip_end"]
            and not board.is_check()
            and not board.is_capture(move)
            and move.promotion is None
        )
        if keep:
            cp = parse_score_cp(node.comment or "")
            if cp is not None and abs(cp) <= opts["max_cp"]:
                if board.turn == chess.BLACK:
                    cp = -cp
                candidates.append((board.fen(), cp, result))
        board.push(move)

    if len(candidates) > opts["max_per_game"]:
        moves_str = " ".join(nd.move.uci() for nd in nodes)
        digest = int(hashlib.md5(moves_str.encode()).hexdigest(), 16)
        rng = random.Random(opts["seed"] ^ (digest & 0xFFFFFFFFFFFF))
        candidates = rng.sample(candidates, opts["max_per_game"])
    return candidates


def worker(task):
    path, start, end, opts = task
    with open(path, "rb") as f:
        f.seek(start)
        data = f.read(end - start)
    sio = io.StringIO(data.decode("utf-8", errors="replace"))
    rows, games = [], 0
    while True:
        try:
            game = chess.pgn.read_game(sio)
        except Exception:
            continue
        if game is None:
            break
        games += 1
        rows.extend(process_game(game, opts))
    return games, rows


def fen_hash(fen: str) -> int:
    key = " ".join(fen.split()[:4])
    return int(hashlib.md5(key.encode()).hexdigest()[:16], 16)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("pgns", nargs="+")
    ap.add_argument("--out", required=True)
    ap.add_argument("--append", action="store_true",
                    help="append to --out instead of overwriting (dedup is "
                         "per-invocation only; prefer one invocation with all PGNs)")
    ap.add_argument("--max-per-game", type=int, default=24)
    ap.add_argument("--skip-start", type=int, default=8)
    ap.add_argument("--skip-end", type=int, default=6)
    ap.add_argument("--max-cp", type=int, default=3200)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--jobs", type=int, default=0, help="workers (0 = CPUs-1)")
    ap.add_argument("--no-dedup", action="store_true")
    args = ap.parse_args()

    jobs = args.jobs if args.jobs > 0 else max(1, (os.cpu_count() or 2) - 1)
    opts = {k: getattr(args, k) for k in
            ("max_per_game", "skip_start", "skip_end", "max_cp", "seed")}

    seen: set[int] = set()
    total_games = total_raw = total_kept = 0
    wsum = 0.0
    mode = "a" if args.append else "w"
    out_dir = os.path.dirname(os.path.abspath(args.out))
    if out_dir:
        os.makedirs(out_dir, exist_ok=True)

    with open(args.out, mode, encoding="utf-8", newline="\n") as out:
        for pgn in args.pgns:
            if not os.path.isfile(pgn):
                sys.exit(f"ERROR: PGN not found: {pgn}")
            filesize = os.path.getsize(pgn)
            nominal = [filesize * i // jobs for i in range(jobs + 1)]
            starts = [next_game_offset(pgn, x, filesize) for x in nominal]
            starts[0], starts[-1] = 0, filesize
            tasks = [(pgn, starts[i], starts[i + 1], opts)
                     for i in range(jobs) if starts[i + 1] > starts[i]]
            print(f"{pgn}  ({filesize/1e6:.0f} MB, {len(tasks)} workers)")
            with Pool(len(tasks)) as pool:
                results = pool.map(worker, tasks)
            for games, rows in results:
                total_games += games
                total_raw += len(rows)
                for fen, cp, res in rows:
                    if not args.no_dedup:
                        h = fen_hash(fen)
                        if h in seen:
                            continue
                        seen.add(h)
                    out.write(f"{fen} | {cp} | {res}\n")
                    total_kept += 1
                    wsum += res

    print()
    print(f"  Games processed : {total_games:,}")
    print(f"  Sampled rows    : {total_raw:,}")
    print(f"  Written (unique): {total_kept:,}")
    if total_kept:
        print(f"  Result mix      : white {100*wsum/total_kept:.1f}% of points")
    print(f"\nWrote {args.out}")


if __name__ == "__main__":
    main()
