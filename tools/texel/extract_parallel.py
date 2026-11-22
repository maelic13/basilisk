#!/usr/bin/env python3
"""Parallel, deterministic Texel extractor for large Basilisk self-play PGNs.

The production path for :mod:`extract`, under the same contract,
basilisk-texel-rows-v1: worker processes parse byte-aligned PGN ranges and
decide every game with ``extract.game_rows``, and the parent feeds the results
to the same reservoirs, in file order, and publishes through
``extract.finish``. Its output is byte-identical to the sequential
extractor's on the same inputs and options.
"""

from __future__ import annotations

import argparse
import io
import os
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
    """Every game in one byte range, decided by the contract, in file order.
    None stands for a game that did not parse."""
    path, start, end, opts = task
    with open(path, "rb") as pgn:
        pgn.seek(start)
        text = pgn.read(end - start).decode("utf-8", errors="replace")
    stream = io.StringIO(text)
    output: list[seq.GameRows | None] = []
    while True:
        try:
            game = chess.pgn.read_game(stream)
        except Exception:  # python-chess raises several types
            output.append(None)
            continue
        if game is None:
            break
        output.append(None if game.errors else seq.game_rows(game, opts))
    return output


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("pgn", nargs="+", help="PGN files")
    seq.add_contract_arguments(parser)
    parser.add_argument("--jobs", default=0, type=int, help="worker processes (0 = CPUs-1)")
    parser.add_argument("--audit-only", action="store_true",
                        help="report the accounting and quotas without publishing")
    args = parser.parse_args()
    seq.check_contract_arguments(args, parser)

    paths = seq.iter_pgn_paths(args.pgn)
    opts = seq.contract_options(args)
    jobs = args.jobs if args.jobs > 0 else max(1, (os.cpu_count() or 2) - 1)
    tasks = []
    for path in paths:
        filesize = path.stat().st_size
        nominal = [filesize * index // jobs for index in range(jobs + 1)]
        starts = [next_game_offset(str(path), offset, filesize) for offset in nominal]
        starts[0], starts[-1] = 0, filesize
        tasks.extend((str(path), starts[index], starts[index + 1], opts)
                     for index in range(jobs) if starts[index + 1] > starts[index])

    collector = seq.Collector(args.target_train, args.phase_weights, args.validation_pct,
                              args.test_pct, args.seed)
    print(f"Parallel extract: {len(paths)} file(s), {len(tasks)} ranges, {jobs} workers")
    print(f"Rows: {collector.counts} | skip_start={args.skip_start}, skip_end={args.skip_end}, "
          f"max/phase/game={args.max_per_phase_per_game}, "
          f"quiet_filter={'on' if args.quiet_filter else 'OFF'}")

    with Pool(min(jobs, len(tasks))) as pool:
        # imap keeps the ranges in file order, so the reservoirs see the games
        # in the order the sequential extractor does.
        for decided in pool.imap(worker, tasks):
            for rows in decided:
                if rows is None:
                    collector.parse_error()
                else:
                    collector.add(rows)

    if args.audit_only:
        status = seq.report(collector, args.max_reject_pct)
        seq.report_quotas(collector)
        short = collector.short()
        if short:
            print(f"Audit: phase quotas not met for {short}.", file=sys.stderr)
            return 2
        print("Audit complete: every quota is available; nothing was published.")
        return status
    return seq.finish(collector, args, paths)


if __name__ == "__main__":
    raise SystemExit(main())
