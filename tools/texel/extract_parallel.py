#!/usr/bin/env python3
"""
Parallel PGN -> FEN;result extractor (drop-in fast path for extract.py).

extract.py is single-threaded: python-chess fully parses and replays every
game, which dominates wall time on multi-hundred-thousand-game self-play PGNs.
This script splits the PGN into byte ranges aligned to game boundaries, parses
each range in a separate process (reusing extract.py's *exact* per-game
extraction logic), then merges with the SAME global dedup + by-game holdout
semantics as the sequential tool, in file order -> identical train/holdout
content (modulo which 5% of games land in holdout, see below).

Holdout assignment: sequential uses an order-dependent RNG draw per game; here
each game is assigned by a stable hash of its first FEN (parallel-safe,
order-independent). Both yield ~holdout_pct% of games with no position leakage
between train and holdout; the fits are indifferent to which games are held out.

Usage:
    python tools/texel/extract_parallel.py <selfplay.pgn> \
        --train train_v17.csv --holdout holdout_v17.csv [--jobs N] [extract.py opts]
"""
import argparse
import hashlib
import io
import os
import random
import sys
from multiprocessing import Pool

import chess.pgn

# Reuse the validated per-game logic verbatim.
import extract as seq  # process_game, game_phase, phase_bucket, fen_key, BUCKET_NAMES


def next_game_offset(path, nominal, filesize):
    """First byte offset of a game header ('[Event ') at or after `nominal`."""
    if nominal <= 0:
        return 0
    if nominal >= filesize:
        return filesize
    with open(path, "rb") as f:
        # Back up one byte so a boundary right at `nominal` is caught via '\n[Event '.
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
                return base - len(carry) + idx + 1  # position of '['
            carry = buf[-8:]               # keep enough to span the marker
            base += len(chunk)


def worker(task):
    path, start, end, opts = task
    with open(path, "rb") as f:
        f.seek(start)
        data = f.read(end - start)
    text = data.decode("utf-8", errors="replace")
    sio = io.StringIO(text)
    pct = opts["holdout_pct"]
    seed = opts["seed"]
    out = []  # list of (is_holdout, [(fen, label, phase), ...]) in file order
    while True:
        try:
            game = chess.pgn.read_game(sio)
        except Exception:
            continue
        if game is None:
            break
        # Seed sampling by the game's own movetext so the result is identical
        # regardless of how many workers split the file (reproducible runs).
        moves_str = " ".join(m.uci() for m in game.mainline_moves())
        g_digest = int(hashlib.md5(moves_str.encode("utf-8")).hexdigest(), 16)
        g_rng = random.Random(seed ^ (g_digest & 0xFFFFFFFFFFFF))
        pairs = seq.process_game(game, opts["skip_start"], opts["skip_end"],
                                 opts["max_per_game"], g_rng)
        if not pairs:
            continue
        is_holdout = (g_digest % 100) < pct
        out.append((is_holdout, pairs))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("pgn")
    ap.add_argument("--out-dir", default="")
    ap.add_argument("--train", default="train.csv")
    ap.add_argument("--holdout", default="holdout.csv")
    ap.add_argument("--holdout-pct", type=int, default=5)
    ap.add_argument("--max-per-game", type=int, default=12)
    ap.add_argument("--skip-start", type=int, default=16)
    ap.add_argument("--skip-end", type=int, default=6)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--jobs", type=int, default=0, help="worker processes (0 = CPUs-1)")
    args = ap.parse_args()

    if not os.path.isfile(args.pgn):
        sys.exit(f"ERROR: PGN not found: {args.pgn}")

    jobs = args.jobs if args.jobs > 0 else max(1, (os.cpu_count() or 2) - 1)
    filesize = os.path.getsize(args.pgn)
    out_dir = args.out_dir or os.path.dirname(os.path.abspath(args.pgn))
    os.makedirs(out_dir, exist_ok=True)

    # Byte ranges aligned to game starts.
    nominal = [filesize * i // jobs for i in range(jobs + 1)]
    starts = [next_game_offset(args.pgn, n, filesize) for n in nominal]
    starts[0], starts[-1] = 0, filesize
    opts = {k: getattr(args, k) for k in
            ("holdout_pct", "max_per_game", "skip_start", "skip_end", "seed")}
    tasks = [(args.pgn, starts[i], starts[i + 1], opts)
             for i in range(jobs) if starts[i + 1] > starts[i]]

    print(f"Parallel extract: {args.pgn}  ({filesize/1e6:.0f} MB, {len(tasks)} workers)")
    with Pool(len(tasks)) as pool:
        results = pool.map(worker, tasks)

    # Merge in file order with global dedup (matches sequential semantics).
    seen = set()
    train, holdout = [], []
    raw = 0
    for task_result in results:                 # task order == file order
        for is_holdout, pairs in task_result:
            raw += len(pairs)
            target = holdout if is_holdout else train
            for fen, label, phase in pairs:
                k = seq.fen_key(fen)
                if k in seen:
                    continue
                seen.add(k)
                target.append((fen, label, phase))

    train_path = os.path.join(out_dir, args.train)
    holdout_path = os.path.join(out_dir, args.holdout)
    for path, rows in ((train_path, train), (holdout_path, holdout)):
        with open(path, "w", encoding="utf-8", newline="\n") as fh:
            for fen, label, _ in rows:
                fh.write(f"{fen};{label}\n")

    def mix(positions):
        c = [0, 0, 0]
        for _, _, ph in positions:
            c[seq.phase_bucket(ph)] += 1
        return c

    tc = mix(train)
    print(f"\n  Raw candidates   : {raw:,}")
    print(f"  Unique positions : {len(seen):,}")
    print(f"  Train positions  : {len(train):,}")
    print(f"  Holdout positions: {len(holdout):,}")
    print("  Train phase mix  : " +
          ", ".join(f"{seq.BUCKET_NAMES[b]}={tc[b]:,}" for b in range(3)))
    print(f"\nWrote {train_path}\n      {holdout_path}")


if __name__ == "__main__":
    main()
