#!/usr/bin/env python3
"""Consecutive-depth branching profile (imported method, Manta MAN-S23).

Why not nodes^(1/depth): that estimator folds the fixed cost of the first
plies into every reading, so it understates branching and is not comparable
across engines with different opening overheads. The ratio between consecutive
depths is the quantity that actually decides how deep a node budget reaches.

Two rules the source project learned the hard way and we inherit:

  HASH IS PART OF THE MEASUREMENT. Manta scored 171,653,746 nodes at depth 12
  with 16 MiB against 159,169,542 with 64 MiB, ~8%. Their first baseline
  spliced two sizes and had to be retracted. Ours had the same defect: Basilisk
  defaults to Hash 64 and the Stockfish-based oracle to 16. Every arm here is
  set explicitly and the size is printed with the result.

  AN ENDPOINT MEASURE INHERITS ITS LAST DEPTH. Manta's b(4-12) was decided by
  one position of forty that exploded 41.5% at depth 12. Per-position ratios
  and a median are reported alongside the aggregate so a single position cannot
  carry the verdict.

Each depth runs in a FRESH process so no ordering or table state carries over.
"""
import argparse, pathlib, re, statistics, subprocess, sys, time

REPO = pathlib.Path(__file__).resolve().parents[2]
DEPTH_LINE = re.compile(r"^info depth (\d+).*?\bnodes (\d+)")


def uci(engine, cmds, timeout=900):
    p = subprocess.Popen([str(engine)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                         stderr=subprocess.DEVNULL, text=True, bufsize=1)
    out = []
    try:
        p.stdin.write("\n".join(cmds) + "\n"); p.stdin.flush()
        dl = time.monotonic() + timeout
        for line in p.stdout:
            out.append(line)
            if line.startswith("bestmove"):
                break
            if time.monotonic() > dl:
                raise TimeoutError(str(engine))
        try:
            p.stdin.write("quit\n"); p.stdin.flush(); p.wait(timeout=10)
        except Exception:
            pass
    finally:
        if p.poll() is None:
            p.kill(); p.wait(timeout=10)
    return "".join(out)


def nodes_at(engine, fen, depth, hash_mb, opts):
    txt = uci(engine, [*opts, "setoption name Hash value %d" % hash_mb,
                       "ucinewgame", "position fen " + fen, "go depth %d" % depth])
    n = 0
    for line in txt.splitlines():
        m = DEPTH_LINE.match(line.strip())
        if m and int(m.group(1)) == depth:
            n = int(m.group(2))
    return n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--engine", required=True)
    ap.add_argument("--label", default=None)
    ap.add_argument("--option", action="append", default=[])
    ap.add_argument("--positions", default=str(REPO / "tools" / "diag" / "suite_v1.epd"))
    ap.add_argument("--limit", type=int, default=20, help="positions to use")
    ap.add_argument("--min-depth", type=int, default=4)
    ap.add_argument("--max-depth", type=int, default=11)
    ap.add_argument("--hash", type=int, default=64)
    args = ap.parse_args()

    fens = [l.strip() for l in open(args.positions, encoding="utf-8")
            if l.strip() and not l.startswith("#")][:args.limit]
    opts = ["setoption name %s value %s" % tuple(o.split("=", 1)) for o in args.option]
    engine = str(pathlib.Path(args.engine).resolve())
    label = args.label or pathlib.Path(args.engine).stem

    totals, per_pos = {}, []
    for fen in fens:
        seq = {}
        for d in range(args.min_depth, args.max_depth + 1):
            n = nodes_at(engine, fen, d, args.hash, opts)
            seq[d] = n
            totals[d] = totals.get(d, 0) + n
        lo, hi = seq[args.min_depth], seq[args.max_depth]
        if lo > 0 and hi > 0:
            per_pos.append((hi / lo) ** (1.0 / (args.max_depth - args.min_depth)))

    span = args.max_depth - args.min_depth
    agg = (totals[args.max_depth] / totals[args.min_depth]) ** (1.0 / span)

    print("%s  |  Hash %d MiB  |  %d positions  |  depths %d-%d"
          % (label, args.hash, len(fens), args.min_depth, args.max_depth))
    print()
    print("%6s %16s %10s" % ("depth", "nodes", "ratio"))
    print("-" * 36)
    prev = None
    for d in range(args.min_depth, args.max_depth + 1):
        r = "%.3f" % (totals[d] / prev) if prev else "-"
        print("%6d %16s %10s" % (d, format(totals[d], ","), r))
        prev = totals[d]
    print()
    print("  b(%d-%d) aggregate       %.3f" % (args.min_depth, args.max_depth, agg))
    if per_pos:
        print("  b(%d-%d) per-position    median %.3f   min %.3f   max %.3f"
              % (args.min_depth, args.max_depth, statistics.median(per_pos),
                 min(per_pos), max(per_pos)))
        print("  (median guards against one position carrying the aggregate)")


if __name__ == "__main__":
    main()
