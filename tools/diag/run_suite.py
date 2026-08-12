#!/usr/bin/env python3
"""Run the Phase-5.2 differential suite (PLAN 5.2).

Two things, because 5.2 has two jobs.

1. INTERNAL BREAKDOWN. Run Basilisk over the fixed suite at fixed depth with
   `Diag` on and aggregate the counters. This says WHERE our tree width is
   created. Fixed depth, not fixed time, so the numbers are deterministic and
   comparable across runs and machines.

2. DIFFERENTIAL vs THE ORACLE. Run both Basilisk and the 5.1 oracle at a fixed
   NODE budget and record the depth each reaches. Depth-at-equal-nodes is the
   direct expression of the BAS-O03 finding (EBF 2.20 against 1.61) and needs
   no instrumentation on the Stockfish side at all, which is what makes the
   comparison possible in the first place.

Counters explain a candidate; they never accept one. Only a registered SPRT
accepts (PLAN cluster discipline).

Usage:
  python tools/diag/run_suite.py --engine build/release/basilisk.exe --depth 14
  python tools/diag/run_suite.py --engine <bas> --oracle <oracle.exe> --nodes 300000
"""

import argparse
import json
import pathlib
import re
import subprocess
import sys
import time

REPO = pathlib.Path(__file__).resolve().parents[2]

# Counters that are meaningful as a ratio rather than a raw total. Raw sums are
# dominated by whichever positions happened to search the most nodes.
DERIVED = [
    ("first_move_cutoff_pct", lambda c: pct(c.get("fail_high_first"), c.get("fail_highs"))),
    ("mean_cutoff_index",     lambda c: ratio(c.get("fail_high_index_sum"), c.get("fail_highs"))),
    ("lmr_applied_pct",       lambda c: pct(c.get("lmr_applied"), c.get("lmr_eligible"))),
    ("lmr_mean_reduction",    lambda c: ratio(c.get("lmr_reduction_plies"), c.get("lmr_applied"))),
    ("lmr_research_pct",      lambda c: pct(c.get("lmr_researched"), c.get("lmr_applied"))),
    ("lmr_clamp0_pct",        lambda c: pct(c.get("lmr_clamped_zero"), c.get("lmr_eligible"))),
    ("cutoff_src_quiet_pct",  lambda c: pct(c.get("cutoff_src_quiet"), c.get("fail_highs"))),
]


def pct(a, b):
    return 100.0 * a / b if a is not None and b else 0.0


def ratio(a, b):
    return a / b if a is not None and b else 0.0


def load_suite(path):
    out = []
    for line in path.read_text(encoding="utf-8").splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            out.append(line)
    return out


def uci_run(engine, commands, timeout=600):
    """Drive one UCI search interactively and return everything printed.

    Commands must NOT be piped in with a trailing `quit`. Stockfish -- and so
    the 5.1 oracle -- reads the next line while the search runs and aborts on
    `quit`, returning the first root move after a token search. On the first
    attempt here that silently produced depth 0 for all 107 oracle positions
    with no error anywhere: the runner looked like it worked and the data was
    empty. So write the commands, read until `bestmove`, and only then quit.
    """
    proc = subprocess.Popen([str(engine)], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                            text=True, bufsize=1)
    out = []
    try:
        proc.stdin.write("\n".join(commands) + "\n")
        proc.stdin.flush()
        deadline = time.monotonic() + timeout
        for line in proc.stdout:
            out.append(line)
            if line.startswith("bestmove"):
                break
            if time.monotonic() > deadline:
                raise TimeoutError(f"{engine} did not finish within {timeout}s")
        # Best effort: the output we need is already captured, and the engine
        # may have closed its pipe on its own. A failure to say goodbye is not
        # a failure of the measurement, and the finally-block reaps it anyway.
        try:
            proc.stdin.write("quit\n")
            proc.stdin.flush()
            proc.wait(timeout=10)
        except (OSError, ValueError, subprocess.TimeoutExpired):
            pass
    finally:
        if proc.poll() is None:
            proc.kill()
            proc.wait(timeout=10)
    return "".join(out)


# Only the canonical `info string diag kv name=value ...` lines are parsed.
# The prose lines alongside them carry percentages and floats that are correct
# for one search and meaningless when summed across a suite, so they are
# deliberately ignored rather than heuristically stripped.
KV_LINE = re.compile(r"^info string diag kv (.*)$")
KV_TOKEN = re.compile(r"([a-z_0-9]+)=(-?\d+)")


def parse_diag(text):
    counters = {}
    for line in text.splitlines():
        m = KV_LINE.match(line.strip())
        if not m:
            continue
        for key, val in KV_TOKEN.findall(m.group(1)):
            counters[key] = counters.get(key, 0) + int(val)
    return counters


DEPTH_LINE = re.compile(r"^info depth (\d+).*?\bnodes (\d+)")


def last_depth_nodes(text):
    depth = nodes = 0
    for line in text.splitlines():
        m = DEPTH_LINE.match(line.strip())
        if m:
            depth, nodes = int(m.group(1)), int(m.group(2))
    return depth, nodes


def run_internal(engine, fens, depth):
    total = {}
    print(f"internal breakdown: {len(fens)} positions at depth {depth}")
    for i, fen in enumerate(fens, 1):
        out = uci_run(engine, [
            "setoption name Diag value true",
            "ucinewgame",
            f"position fen {fen}",
            f"go depth {depth}",
        ])
        for k, v in parse_diag(out).items():
            total[k] = total.get(k, 0) + v
        if i % 25 == 0:
            print(f"  {i}/{len(fens)}")
    return total


def run_differential(engine, oracle, fens, nodes, oracle_opts):
    rows = []
    print(f"differential: {len(fens)} positions at {nodes:,} nodes")
    for i, fen in enumerate(fens, 1):
        a = uci_run(engine, ["ucinewgame", f"position fen {fen}", f"go nodes {nodes}"])
        b = uci_run(oracle, oracle_opts + ["ucinewgame", f"position fen {fen}",
                                           f"go nodes {nodes}"])
        da, na = last_depth_nodes(a)
        db, nb = last_depth_nodes(b)
        rows.append({"fen": fen, "basilisk_depth": da, "basilisk_nodes": na,
                     "oracle_depth": db, "oracle_nodes": nb})
        if i % 25 == 0:
            print(f"  {i}/{len(fens)}")
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--engine", required=True)
    ap.add_argument("--oracle", default=None,
                    help="5.1 oracle exe; enables the differential pass")
    ap.add_argument("--oracle-hce", default="true",
                    help="'true' = Basilisk HCE under Stockfish search (isolates search)")
    ap.add_argument("--suite", default=str(REPO / "tools" / "diag" / "suite_v1.epd"))
    ap.add_argument("--depth", type=int, default=14)
    ap.add_argument("--nodes", type=int, default=300000)
    ap.add_argument("--out", default=None)
    args = ap.parse_args()

    fens = load_suite(pathlib.Path(args.suite))
    if not fens:
        sys.exit("suite is empty")

    report = {"suite": pathlib.Path(args.suite).name, "positions": len(fens),
              "depth": args.depth}

    counters = run_internal(pathlib.Path(args.engine), fens, args.depth)
    report["counters"] = counters
    report["derived"] = {name: fn(counters) for name, fn in DERIVED}

    print("\n--- internal breakdown ---")
    for name, _ in DERIVED:
        print(f"  {name:24} {report['derived'][name]:10.3f}")
    # eligible = applied + clamped_zero + sum(blocked_*). If this fails the
    # counters are internally inconsistent and nothing derived from them can be
    # trusted, so it is checked on every run rather than assumed.
    elig = counters.get("lmr_eligible", 0)
    parts = (counters.get("lmr_applied", 0) + counters.get("lmr_clamped_zero", 0)
             + counters.get("lmr_blocked_depth", 0)
             + counters.get("lmr_blocked_searched", 0)
             + counters.get("lmr_blocked_in_check", 0)
             + counters.get("lmr_blocked_movetype", 0)
             + counters.get("lmr_blocked_gives_check", 0))
    ident_ok = elig > 0 and elig == parts
    print(f"  lmr accounting identity  {'HOLDS' if ident_ok else f'BROKEN {elig} != {parts}'}")
    report["lmr_identity_ok"] = ident_ok

    if args.oracle:
        opts = [f"setoption name Use Basilisk HCE value {args.oracle_hce}"]
        rows = run_differential(pathlib.Path(args.engine), pathlib.Path(args.oracle),
                                fens, args.nodes, opts)
        report["differential"] = rows
        ok = [r for r in rows if r["basilisk_depth"] and r["oracle_depth"]]
        if not ok:
            sys.exit("differential produced no usable rows - one engine reported "
                     "depth 0 everywhere. Do not report this run as a result.")
        if ok:
            mb = sum(r["basilisk_depth"] for r in ok) / len(ok)
            mo = sum(r["oracle_depth"] for r in ok) / len(ok)
            report["mean_depth_basilisk"] = mb
            report["mean_depth_oracle"] = mo
            print("\n--- differential at equal nodes ---")
            print(f"  mean depth  basilisk {mb:6.2f}   oracle {mo:6.2f}   delta {mo - mb:+.2f}")
            print("  (positive delta = the reference converts the same nodes into more depth)")

    if args.out:
        pathlib.Path(args.out).write_text(json.dumps(report, indent=2), encoding="utf-8")
        print(f"\nwrote {args.out}")


if __name__ == "__main__":
    main()
