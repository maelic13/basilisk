#!/usr/bin/env python3
"""Audit a FEN position store in one sequential pass, without writing to it.

    python tools/texel/audit_store.py <positions.txt> --out <report.json>

The store is one FEN (or EPD) per line. The pass reports:

* its SHA-256, bytes and lines;
* the rows that parse as positions (placement, side, castling, en passant);
* the material-phase distribution on the evaluator's scale (N = B = 1,
  R = 2, Q = 4, capped at 24), and its five extraction buckets;
* exact duplicates on the four fields that identify a position, counted from
  64-bit hashes, so a ~125M-line store fits in memory.

It never seeks, so a slow or spinning drive is read at its sequential rate.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
import time
from pathlib import Path

PHASE_W = {"n": 1, "b": 1, "r": 2, "q": 4}
BUCKETS = (("opening", 20, 24), ("early_mid", 14, 19), ("middlegame", 8, 13),
           ("endgame", 3, 7), ("deep_endgame", 0, 2))


def phase_of(placement: str) -> int:
    total = 0
    for ch in placement:
        total += PHASE_W.get(ch.lower(), 0)
    return min(24, total)


def bucket_of(phase: int) -> str:
    for name, lo, hi in BUCKETS:
        if lo <= phase <= hi:
            return name
    raise ValueError(phase)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("store", type=Path)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--chunk-mb", type=int, default=64)
    args = parser.parse_args()
    try:
        import numpy as np
    except ImportError:
        print("error: needs numpy", file=sys.stderr)
        return 2

    digest = hashlib.sha256()
    phases = [0] * 25
    lines = parsed = malformed = 0
    keys = []           # arrays of 64-bit position hashes, one per chunk
    pending = []
    start = time.monotonic()
    size = args.store.stat().st_size
    done_bytes = 0
    carry = b""
    with args.store.open("rb") as store:
        while True:
            block = store.read(args.chunk_mb << 20)
            if not block:
                break
            digest.update(block)
            done_bytes += len(block)
            data = carry + block
            cut = data.rfind(b"\n")
            if cut < 0:
                carry = data
                continue
            carry = data[cut + 1:]
            for raw in data[:cut].split(b"\n"):
                lines += 1
                fields = raw.split()
                if len(fields) < 4 or fields[1] not in (b"w", b"b"):
                    malformed += 1
                    continue
                parsed += 1
                placement = fields[0].decode("ascii", "replace")
                phases[phase_of(placement)] += 1
                key = b" ".join(fields[:4])
                pending.append(int.from_bytes(hashlib.blake2b(key, digest_size=8).digest(), "little"))
            if len(pending) >= 4_000_000:
                keys.append(np.array(pending, dtype=np.uint64))
                pending = []
            print(f"\r{done_bytes / size:6.1%}  {lines:,} lines  {time.monotonic() - start:,.0f} s",
                  end="", file=sys.stderr, flush=True)
    if carry.strip():
        lines += 1
        fields = carry.split()
        if len(fields) >= 4 and fields[1] in (b"w", b"b"):
            parsed += 1
            phases[phase_of(fields[0].decode("ascii", "replace"))] += 1
            pending.append(int.from_bytes(hashlib.blake2b(b" ".join(fields[:4]), digest_size=8).digest(),
                                          "little"))
        else:
            malformed += 1
    if pending:
        keys.append(np.array(pending, dtype=np.uint64))
    print(file=sys.stderr)
    all_keys = np.concatenate(keys) if keys else np.zeros(0, dtype=np.uint64)
    distinct = int(np.unique(all_keys).size)
    buckets = {name: sum(phases[lo:hi + 1]) for name, lo, hi in BUCKETS}
    report = {
        "store": str(args.store),
        "bytes": size,
        "sha256": digest.hexdigest().upper(),
        "lines": lines,
        "positions": parsed,
        "malformed": malformed,
        "distinct_positions_4_fields": distinct,
        "duplicate_share": round(1 - distinct / parsed, 6) if parsed else None,
        "phase_histogram": phases,
        "phase_buckets": buckets,
        "phase_bucket_shares": {k: round(v / parsed, 6) for k, v in buckets.items()} if parsed else {},
        "seconds": round(time.monotonic() - start, 1),
    }
    args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({k: report[k] for k in ("sha256", "lines", "positions", "malformed",
                                             "distinct_positions_4_fields", "duplicate_share",
                                             "phase_bucket_shares", "seconds")}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
