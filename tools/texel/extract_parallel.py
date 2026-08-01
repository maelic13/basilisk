#!/usr/bin/env python3
"""Parallel, deterministic Texel extractor for large Basilisk self-play PGNs.

This is the production path for :mod:`extract`.  It preserves the same five
phase, per-game sampling, quiet-filter, global-dedup and exact-reservoir
contract while parsing byte-aligned PGN ranges in worker processes.
"""

from __future__ import annotations

import argparse
import io
import os
import random
import sys
from multiprocessing import Pool
from pathlib import Path

import chess.pgn

import extract as seq


def next_game_offset(path: str, nominal: int, filesize: int) -> int:
    """Return the first ``[Event `` game header at or after nominal."""
    if nominal <= 0:
        return 0
    if nominal >= filesize:
        return filesize
    with open(path, "rb") as pgn:
        pos = max(0, nominal - 1)
        pgn.seek(pos)
        carry = b""
        base = pos
        while True:
            chunk = pgn.read(1 << 16)
            if not chunk:
                return filesize
            buf = carry + chunk
            index = buf.find(b"\n[Event ")
            if index != -1:
                return base - len(carry) + index + 1
            carry = buf[-8:]
            base += len(chunk)


def worker(task):
    path, start, end, opts = task
    with open(path, "rb") as pgn:
        pgn.seek(start)
        text = pgn.read(end - start).decode("utf-8", errors="replace")
    stream = io.StringIO(text)
    output = []
    games = skipped = quiet_rejected = raw = parse_errors = 0
    holdout_cut = round(opts["holdout_pct"] * 100)

    while True:
        try:
            game = chess.pgn.read_game(stream)
        except Exception:
            parse_errors += 1
            continue
        if game is None:
            break
        games += 1
        digest = seq.game_digest(game)
        rng = random.Random(opts["seed"] ^ digest)
        pairs, rejected = seq.process_game(
            game,
            opts["skip_start"],
            opts["skip_end"],
            opts["max_per_phase_per_game"],
            opts["max_per_game"],
            opts["quiet_filter"],
            rng,
        )
        quiet_rejected += rejected
        if not pairs:
            skipped += 1
            continue
        result = seq.RESULT_MAP[game.headers["Result"]]
        rows = [(fen, result, bucket) for fen, bucket in pairs]
        raw += len(rows)
        output.append((digest % 10_000 < holdout_cut, rows))

    return output, {
        "games": games,
        "skipped": skipped,
        "quiet_rejected": quiet_rejected,
        "raw": raw,
        "parse_errors": parse_errors,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("pgn")
    parser.add_argument("--out-dir", default="")
    parser.add_argument("--train", default="train.csv")
    parser.add_argument("--holdout", default="holdout.csv")
    parser.add_argument("--target-train", default=3_500_000, type=int, metavar="N")
    parser.add_argument("--phase-weights", default=seq.parse_phase_weights("1,1,1,1,1"),
                        type=seq.parse_phase_weights, metavar="A,B,C,D,E")
    parser.add_argument("--holdout-pct", default=5.0, type=float)
    parser.add_argument("--max-per-phase-per-game", default=8, type=int)
    parser.add_argument("--max-per-game", default=0, type=int)
    parser.add_argument("--skip-start", default=0, type=int)
    parser.add_argument("--skip-end", default=6, type=int)
    parser.add_argument("--seed", default=42, type=int)
    parser.add_argument("--jobs", default=0, type=int, help="worker processes (0 = CPUs-1)")
    parser.add_argument("--no-quiet-filter", dest="quiet_filter", action="store_false")
    parser.add_argument("--audit-only", action="store_true",
                        help="report exact eligible counts without publishing CSVs")
    args = parser.parse_args()

    if not os.path.isfile(args.pgn):
        parser.error(f"PGN not found: {args.pgn}")
    if args.target_train <= 0:
        parser.error("--target-train must be positive")
    if not 0.0 <= args.holdout_pct < 100.0:
        parser.error("--holdout-pct must be in [0,100)")
    if args.max_per_phase_per_game <= 0:
        parser.error("--max-per-phase-per-game must be positive")
    if args.skip_start < 0 or args.skip_end < 0:
        parser.error("skip counts cannot be negative")

    jobs = args.jobs if args.jobs > 0 else max(1, (os.cpu_count() or 2) - 1)
    filesize = os.path.getsize(args.pgn)
    nominal = [filesize * index // jobs for index in range(jobs + 1)]
    starts = [next_game_offset(args.pgn, offset, filesize) for offset in nominal]
    starts[0], starts[-1] = 0, filesize
    opts = {name: getattr(args, name) for name in (
        "holdout_pct", "max_per_phase_per_game", "max_per_game",
        "skip_start", "skip_end", "seed", "quiet_filter",
    )}
    tasks = [(args.pgn, starts[index], starts[index + 1], opts)
             for index in range(jobs) if starts[index + 1] > starts[index]]

    quotas = seq.allocate(args.target_train, args.phase_weights)
    holdout_total = round(args.target_train * args.holdout_pct / (100.0 - args.holdout_pct))
    holdout_quotas = seq.allocate(holdout_total, args.phase_weights)
    train = [seq.Reservoir(quota, random.Random(args.seed ^ (index + 1) * 0x9E3779B1))
             for index, quota in enumerate(quotas)]
    holdout = [seq.Reservoir(quota, random.Random(args.seed ^ (index + 11) * 0x85EBCA77))
               for index, quota in enumerate(holdout_quotas)]

    print(f"Parallel extract: {args.pgn} ({filesize / 1e6:.0f} MB, {len(tasks)} workers)")
    print("Train quotas: " + ", ".join(
        f"{seq.BUCKET_NAMES[index]}={quota:,}" for index, quota in enumerate(quotas)))
    print(f"skip_start={args.skip_start}, skip_end={args.skip_end}, "
          f"max/phase/game={args.max_per_phase_per_game}, "
          f"quiet_filter={'on' if args.quiet_filter else 'OFF'}")

    seen: set[str] = set()
    totals = {name: 0 for name in ("games", "skipped", "quiet_rejected", "raw", "parse_errors")}
    with Pool(len(tasks)) as pool:
        for task_output, stats in pool.imap(worker, tasks):
            for name, value in stats.items():
                totals[name] += value
            for is_holdout, rows in task_output:
                reservoirs = holdout if is_holdout else train
                for fen, result, bucket in rows:
                    key = seq.fen_key(fen)
                    if key in seen:
                        continue
                    seen.add(key)
                    reservoirs[bucket].offer((fen, result, bucket))

    print(f"\nGames read       : {totals['games']:,}")
    print(f"Games skipped    : {totals['skipped']:,}")
    print(f"Parse errors     : {totals['parse_errors']:,}")
    print(f"Raw candidates   : {totals['raw']:,}")
    print(f"Quiet rejected   : {totals['quiet_rejected']:,}")
    print(f"Unique positions : {len(seen):,}")

    short = []
    for index, name in enumerate(seq.BUCKET_NAMES):
        train_count = len(train[index].items)
        holdout_count = len(holdout[index].items)
        print(f"  {name:13}: train {train_count:,}/{quotas[index]:,} "
              f"holdout {holdout_count:,}/{holdout_quotas[index]:,} "
              f"eligible train={train[index].seen:,} holdout={holdout[index].seen:,}")
        if train_count < quotas[index] or holdout_count < holdout_quotas[index]:
            short.append(index)

    if short:
        print("ERROR: phase quotas not met; existing outputs left untouched.", file=sys.stderr)
        return 2
    if args.audit_only:
        print("Audit complete: all requested quotas are available; no CSVs written.")
        return 0

    rng = random.Random(args.seed)
    train_rows = [(fen, result) for reservoir in train for fen, result, _ in reservoir.items]
    holdout_rows = [(fen, result) for reservoir in holdout for fen, result, _ in reservoir.items]
    rng.shuffle(train_rows)
    rng.shuffle(holdout_rows)
    out_dir = Path(args.out_dir).resolve() if args.out_dir else Path(args.pgn).resolve().parent
    out_dir.mkdir(parents=True, exist_ok=True)
    train_path = out_dir / args.train
    holdout_path = out_dir / args.holdout
    train_tmp = seq.stage_rows(train_path, train_rows)
    holdout_tmp = seq.stage_rows(holdout_path, holdout_rows)
    os.replace(train_tmp, train_path)
    os.replace(holdout_tmp, holdout_path)
    print(f"Wrote {len(train_rows):,} train -> {train_path}")
    print(f"Wrote {len(holdout_rows):,} holdout -> {holdout_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
