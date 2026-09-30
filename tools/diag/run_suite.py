#!/usr/bin/env python3
"""Run the Phase-5.2 differential suite (PLAN 5.2).

Two things, because 5.2 has two jobs.

1. INTERNAL BREAKDOWN. Run Basilisk over the fixed suite at fixed depth with
   `Diag` on and aggregate the counters. This says WHERE our tree width is
   created. Fixed depth, not fixed time, so the numbers are deterministic and
   comparable across runs and machines.

2. DIFFERENTIAL vs THE ORACLE. Run both Basilisk and the 5.1 oracle at a fixed
   NODE budget and record the last completed depth and best move from each.
   Depth-at-equal-nodes is the direct expression of the BAS-O03 finding (EBF
   2.20 against 1.61); best-move agreement adds a per-position search-quality
   signal without requiring instrumentation on the Stockfish side.

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


def _counter_units():
    units = {}
    groups = {
        "nodes": "interior_nodes in_check_nodes tt_pv_nodes qs_nodes qs_evasion_nodes",
        "plies": "lmr_reduction_plies",
        "move_index_sum": "fail_high_index_sum",
        "calls": "see_ge_calls gives_check_calls",
        "updates": "hist_cutoff_updates hist_reward_updates",
        "events": """
            check_exts tt_probes tt_hits tt_cutoffs rfp_cuts razor_cuts
            null_tries null_cuts probcut_tries probcut_cuts fut_prunes
            lmp_prunes hist_prunes see_prunes lmr_applied lmr_researched
            tt_stores tt_stores_same_key fail_highs fail_high_first
            cutoff_src_tt cutoff_src_goodcap cutoff_src_quiet cutoff_src_badcap
            lmr_eligible lmr_clamped_zero lmr_clamped_high sing_fired
            sing_double sing_in_check sing_triple sing_ttbeta asp_windows
            asp_fail_low asp_fail_high asp_researches asp_giveup
            hist_prune_tested hist_below_half hist_below_quarter
            hist_below_eighth lmr_blocked_depth lmr_blocked_searched
            lmr_blocked_in_check lmr_blocked_movetype lmr_blocked_gives_check
        """,
    }
    for unit, names in groups.items():
        for name in names.split():
            if name in units:
                raise AssertionError(f"duplicate counter schema entry: {name}")
            units[name] = unit
    if len(units) != 57:
        raise AssertionError(f"counter schema has {len(units)} entries, expected 57")
    return units


CORE_COUNTER_UNITS = _counter_units()


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
            lowered = line.lower()
            if "no such option" in lowered or "unknown option" in lowered:
                raise RuntimeError(f"{engine} rejected an option: {line.strip()}")
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
            if key in counters:
                raise ValueError(f"duplicate diagnostic counter: {key}")
            counters[key] = int(val)
    return counters


def validate_diag(counters):
    missing = sorted(set(CORE_COUNTER_UNITS) - set(counters))
    if missing:
        raise ValueError("missing diagnostic counters: " + ", ".join(missing))

    checks = {
        "tt_hits <= tt_probes": counters["tt_hits"] <= counters["tt_probes"],
        "tt_cutoffs <= tt_hits": counters["tt_cutoffs"] <= counters["tt_hits"],
        "tt_stores_same_key <= tt_stores": (
            counters["tt_stores_same_key"] <= counters["tt_stores"]
        ),
        "check_exts <= in_check_nodes": (
            counters["check_exts"] <= counters["in_check_nodes"]
        ),
        "qs_evasion_nodes <= qs_nodes": (
            counters["qs_evasion_nodes"] <= counters["qs_nodes"]
        ),
        "null_cuts <= null_tries": counters["null_cuts"] <= counters["null_tries"],
        "probcut_cuts <= probcut_tries": (
            counters["probcut_cuts"] <= counters["probcut_tries"]
        ),
        "lmr_researched <= lmr_applied": (
            counters["lmr_researched"] <= counters["lmr_applied"]
        ),
        "cutoff sources == fail highs": (
            counters["cutoff_src_tt"] + counters["cutoff_src_goodcap"]
            + counters["cutoff_src_quiet"] + counters["cutoff_src_badcap"]
            == counters["fail_highs"]
        ),
        "lmr eligibility partitions exactly": (
            counters["lmr_applied"] + counters["lmr_clamped_zero"]
            + counters["lmr_blocked_depth"] + counters["lmr_blocked_searched"]
            + counters["lmr_blocked_in_check"] + counters["lmr_blocked_movetype"]
            + counters["lmr_blocked_gives_check"] == counters["lmr_eligible"]
        ),
        "history thresholds are nested": (
            counters["hist_below_half"] <= counters["hist_below_quarter"]
            <= counters["hist_below_eighth"] <= counters["hist_prune_tested"]
        ),
        "singular subsets are bounded": (
            counters["sing_double"] <= counters["sing_fired"]
            and counters["sing_in_check"] <= counters["sing_fired"]
            and counters["sing_triple"] <= counters["sing_double"]
            and counters["sing_triple"] <= counters["sing_in_check"]
        ),
        "aspiration researches are accounted": (
            counters["asp_researches"] == counters["asp_fail_low"]
            + counters["asp_fail_high"] + counters["asp_giveup"]
        ),
    }
    failed = [name for name, passed in checks.items() if not passed]
    if failed:
        raise ValueError("diagnostic invariant failed: " + "; ".join(failed))
    return checks


DEPTH_LINE = re.compile(r"^info depth (\d+).*?\bnodes (\d+)")
BESTMOVE_LINE = re.compile(r"^bestmove\s+(\S+)")


def last_depth_nodes(text):
    """Return the last completed iteration, excluding aspiration bounds."""
    depth = nodes = 0
    for line in text.splitlines():
        if " lowerbound " in f" {line} " or " upperbound " in f" {line} ":
            continue
        m = DEPTH_LINE.match(line.strip())
        if m:
            depth, nodes = int(m.group(1)), int(m.group(2))
    return depth, nodes


def bestmove(text):
    for line in text.splitlines():
        match = BESTMOVE_LINE.match(line.strip())
        if match:
            return match.group(1)
    return None


def run_internal(engine, fens, depth, options=()):
    total = {}
    records = []
    print(f"internal breakdown: {len(fens)} positions at depth {depth}")
    for i, fen in enumerate(fens, 1):
        out = uci_run(engine, [
            *options,
            "setoption name Diag value true",
            "ucinewgame",
            f"position fen {fen}",
            f"go depth {depth}",
        ])
        counters = parse_diag(out)
        validate_diag(counters)
        records.append({"position": i, "fen": fen, "counters": counters})
        for k, v in counters.items():
            total[k] = total.get(k, 0) + v
        if i % 25 == 0:
            print(f"  {i}/{len(fens)}")
    validate_diag(total)
    return total, records


def run_differential(engine, oracle, fens, nodes, oracle_opts, engine_opts=()):
    rows = []
    print(f"differential: {len(fens)} positions at {nodes:,} nodes")
    for i, fen in enumerate(fens, 1):
        a = uci_run(engine, [*engine_opts, "ucinewgame", f"position fen {fen}",
                             f"go nodes {nodes}"])
        b = uci_run(oracle, oracle_opts + ["ucinewgame", f"position fen {fen}",
                                           f"go nodes {nodes}"])
        da, na = last_depth_nodes(a)
        db, nb = last_depth_nodes(b)
        move_a, move_b = bestmove(a), bestmove(b)
        if move_a is None or move_b is None:
            raise RuntimeError(f"position {i} produced no bestmove from one arm")
        comparable = move_a != "0000" and move_b != "0000"
        rows.append({"position": i, "fen": fen,
                     "basilisk_depth": da, "basilisk_nodes": na,
                     "basilisk_bestmove": move_a,
                     "oracle_depth": db, "oracle_nodes": nb,
                     "oracle_bestmove": move_b,
                     "bestmove_comparable": comparable,
                     "bestmove_agreement": comparable and move_a == move_b})
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
    ap.add_argument("--stride", type=int, default=1,
                    help="diagnostic sampling stride; only exact stride 1 is valid")
    # HASH IS PART OF THE MEASUREMENT. Imported from Manta MAN-S23, which found
    # its own branching baseline had spliced 16 MiB and 64 MiB runs: the same
    # engine scored 171,653,746 nodes at depth 12 with 16 MiB against
    # 159,169,542 with 64 MiB, ~8%. Our own cross-engine runs had the same flaw
    # -- Basilisk defaults to Hash 64 and the Stockfish-based oracle to 16 --
    # so every arm is now set explicitly and the size belongs in the report.
    ap.add_argument("--hash", type=int, default=64,
                    help="Hash MiB applied to EVERY arm; never compare sizes")
    ap.add_argument("--out", default=None)
    ap.add_argument("--option", action="append", default=[], metavar="Name=Value",
                    help="UCI setoption applied to the engine under test "
                         "(repeatable). Search knobs need a TUNE build.")
    args = ap.parse_args()

    if args.stride != 1:
        ap.error("counter ratios require exact sampling stride 1")

    fens = load_suite(pathlib.Path(args.suite))
    if not fens:
        sys.exit("suite is empty")

    report = {"suite": pathlib.Path(args.suite).name, "positions": len(fens),
              "depth": args.depth, "nodes": args.nodes, "hash_mb": args.hash,
              "sampling_stride": 1, "counter_units": CORE_COUNTER_UNITS}

    # A fresh engine process is started per position, so options are re-applied
    # every time rather than once at startup.
    engine_opts = ["setoption name Hash value %d" % args.hash]
    engine_opts += [f"setoption name {o.split('=', 1)[0]} value {o.split('=', 1)[1]}"
                    for o in args.option]
    if engine_opts:
        report["engine_options"] = args.option
        print("engine options: " + ", ".join(args.option))
    counters, per_position = run_internal(
        pathlib.Path(args.engine), fens, args.depth, engine_opts
    )
    report["counters"] = counters
    report["counter_records"] = per_position
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
        opts = ["setoption name Hash value %d" % args.hash,
                f"setoption name Use Basilisk HCE value {args.oracle_hce}"]
        rows = run_differential(pathlib.Path(args.engine), pathlib.Path(args.oracle),
                                fens, args.nodes, opts, engine_opts)
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
        comparable = [r for r in rows if r["bestmove_comparable"]]
        if not comparable:
            sys.exit("differential produced no comparable best moves")
        agreements = sum(r["bestmove_agreement"] for r in comparable)
        report["bestmove_comparable"] = len(comparable)
        report["bestmove_agreements"] = agreements
        report["bestmove_agreement_pct"] = 100.0 * agreements / len(comparable)
        print(f"  best-move agreement {agreements}/{len(comparable)} "
              f"({report['bestmove_agreement_pct']:.2f}%)")

    if args.out:
        pathlib.Path(args.out).write_text(json.dumps(report, indent=2), encoding="utf-8")
        print(f"\nwrote {args.out}")


if __name__ == "__main__":
    main()
