#!/usr/bin/env python3
"""Frozen launcher for C.5.1.1's first stage: BAS-O21 and BAS-M31.

The contract is analysis/c511_residual_2026-11-27.md, §4 to §6, and the two
registrations, docs/experiments/BAS-O21.md and BAS-M31.md. Every read is
3,000 games on UHO `UHO_Lichess_4852_v1.epd` in random order, paired
colour-reversed openings, 1T, Hash 64, margin 20 ms, concurrency 14, no
adjudication, no tablebases, every binary pinned by SHA-256.

  e0        BAS-O21, seed 511001, `3+0.03`: one oracle built around the
            head's src/ (Stockfish 9587eeeb's search). `Use Basilisk
            HCE=false`, the donor's evaluation, against `=true`, the head's,
            `EvalAblationMask=0` on both. A two-engine gauntlet of 1,500
            cycles; the second arm is fixed at 1500 and the first arm's
            rating less 1500 is the read.
  e0n       BAS-O21, the same pair, seed and openings at equal nodes: both
            arms carry `nodestime=1000`, so the search spends nodes where it
            spent milliseconds, and the clock is `8+0.08`
            (tools/colosseum/gauntlet-node-clock.toml): 8,000,000 nodes and
            80,000 a move for each arm, whatever its speed.
  frontier  BAS-M31, seed 511002, `3+0.03`: the head against Stockfish 16
            with `Use NNUE=false`, a fixed match with an external second arm
            and the wrapper's 1% fault allowance for it.
  odds      BAS-M31, the same pair, seed and openings with Stockfish at
            `2.161+0.022`: the head's `3+0.03` is 1.388 times that, BAS-M27's
            ratio of nodes a second. The wrapper gives only the second arm a
            clock of its own, so the odds shorten Stockfish's.

  all: e0, e0n, frontier, odds. A finished read is skipped; the first
  failure stops the run.

Before a read: every binary's hash and manifest. For the oracle, the
evaluation switch must move its `bench 16 1 8 default depth`; for e0n, two
searches of one position on a clock must count the same nodes with
`nodestime` and stay under the node clock. After a dry run the launcher
refuses a launch record in which an arm does not carry exactly its options.

A finished read (result.json) is never extended or replaced. An interrupted
one has only its checkpoint, and the same command resumes it.

--smoke plays a few games per read under other arm names into
tools/results/c511-smoke-<read>, read for its exit, its time losses and the
node clock only.

Usage:
  python tools/run_c511_reads.py --read <e0|e0n|frontier|odds|all> [--dry-run | --smoke]
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import subprocess
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from run_b111_reads import check_binary, gauntlet_args, ps  # noqa: E402
from run_b66_games import Refused, powershell  # noqa: E402
from run_c34_reads import launched_arms, manifest_fields  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[1]
ENGINES = REPO / "tools" / "test_engines"
RESULTS = REPO / "tools" / "results"
CYCLES = 1500
GAMES = 2 * CYCLES
SMOKE_CYCLES = 4
HCE_OPTION = "Use Basilisk HCE"
MASK_OPTION = "EvalAblationMask"
NODESTIME_OPTION = "nodestime"
WIRE_BENCH = "bench 16 1 8 default depth"

# The commit the head and the oracle are built from: master's b5b8817.
REVISION = "b5b8817a5e44bfb402bda253c8936c3668daf719"
HEAD = {"path": ENGINES / "basilisk-c511-head-pext-pgo.exe",
        "sha": "31E63F9A59A2FA2ED3E9235E4123B7609C6198CBE6CE5E0320EF995E240EE989", "bench": 3525148}
# tools/oracle/build_oracle.ps1 with oracle-eval-ablate.patch,
# oracle-eval-transplant.patch, oracle-eval-reference.patch and
# oracle-eval-rebuild.patch, in that order, around REVISION's src/:
# BAS-O16's recipe on the head.
ORACLE = {"path": ENGINES / "oracle-c511-b5b8817-rebuild.exe",
          "sha": "CD44AAC920A8C7DF214157BD5A83749D36E1C67F885C4BF8F0BEEAFF3E25A67F"}
# BAS-M18's Stockfish 16 classical build, every frontier pair's second arm.
SF16 = {"path": pathlib.Path(r"D:\chess\engines\stockfish-16-x64-bmi2-classical-pgo.exe"),
        "sha": "041396A4B14EC25BAE0B57F90E917A17E7FCEB1FBBAB80859EE2EC3774D25442"}

METER_SEED = 511001
FRONTIER_SEED = 511002
# e0n: nodes a millisecond of the oracle's node clock, and the clock that
# makes its budget 8,000,000 nodes and 80,000 a move. The slower arm's
# slowest stretch in BAS-O16's 3,000 games ran 1,050 nodes a millisecond, so
# neither arm can outspend the wall clock.
NODES_PER_MS = 1000
NODE_CLOCK = (8000, 80)
NODE_CLOCK_RUN_FILE = REPO / "tools" / "colosseum" / "gauntlet-node-clock.toml"
# odds: Stockfish 16's clock, `3+0.03` over BAS-M27's ratio of nodes a
# second, its 2.79M over the head's 2.01M.
ODDS_CLOCK = (2161, 22)
CLOCK = (3000, 30)

ORACLE_ARMS = [("Oracle 9587 eval", "false"), ("Oracle head eval", "true")]
ORDER = ["e0", "e0n", "frontier", "odds"]


def run_dir(read: str, smoke: bool) -> pathlib.Path:
    return RESULTS / (f"c511-smoke-{read}" if smoke else f"c511-{read}")


def label(name: str, smoke: bool) -> str:
    return name + (" smoke" if smoke else "")


def oracle_options(read: str, hce: str) -> dict[str, str]:
    options = {HCE_OPTION: hce, MASK_OPTION: "0"}
    if read == "e0n":
        options[NODESTIME_OPTION] = str(NODES_PER_MS)
    return options


def check_head() -> None:
    """The head: its hash, and a manifest naming the commit, a clean tree, the Release flavour and its bench."""
    check_binary(HEAD["path"], HEAD["sha"])
    fields = manifest_fields(HEAD["path"])
    want = {"revision": REVISION, "dirty_diff": "clean", "bench": str(HEAD["bench"]),
            "binary_sha256": HEAD["sha"], "flavor": "pext-pgo-release", "arm_option": "none"}
    for key, value in want.items():
        if fields.get(key, "").upper() != value.upper():
            raise Refused(f"PROVENANCE MISMATCH - {HEAD['path'].with_suffix('.manifest.txt').name} records "
                          f"{key} {fields.get(key)!r}, not {value!r}.")


def oracle_session(commands: list[str], until: str) -> str:
    """Send commands to the oracle, read to the first line starting with `until`, then quit."""
    process = subprocess.Popen([str(ORACLE["path"])], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True)
    try:
        process.stdin.write("\n".join(commands) + "\n")
        process.stdin.flush()
        seen = []
        for line in process.stdout:
            seen.append(line)
            if line.startswith(until):
                break
        else:
            raise Refused(f"ORACLE REFUSED - no {until!r} line after {commands[-1]!r}.")
        process.stdin.write("quit\n")
        process.stdin.flush()
        process.wait(timeout=30)
    finally:
        if process.poll() is None:
            process.kill()
    if process.returncode != 0:
        raise Refused(f"ORACLE REFUSED - exit {process.returncode} after {commands[-1]!r}.")
    return "".join(seen)


def oracle_bench(hce: str) -> int:
    text = oracle_session([f"setoption name {HCE_OPTION} value {hce}", f"setoption name {MASK_OPTION} value 0",
                           WIRE_BENCH], "Nodes/second")
    match = re.search(r"Nodes searched\s*:\s*(\d+)", text)
    if not match:
        raise Refused(f"ORACLE REFUSED - no bench node count with {HCE_OPTION}={hce}.")
    return int(match.group(1))


def clock_search_nodes(hce: str, nodestime: int) -> int:
    """Nodes of one search of the start position on the node clock's `go` line."""
    base, increment = NODE_CLOCK
    text = oracle_session(["uci", f"setoption name {HCE_OPTION} value {hce}", f"setoption name {MASK_OPTION} value 0",
                           "setoption name Hash value 64", f"setoption name {NODESTIME_OPTION} value {nodestime}",
                           "ucinewgame", "position startpos",
                           f"go wtime {base} btime {base} winc {increment} binc {increment}"], "bestmove")
    counts = re.findall(r"^info .*\bnodes (\d+)", text, flags=re.MULTILINE)
    if not counts:
        raise Refused(f"ORACLE REFUSED - no node count from a clock search with {NODESTIME_OPTION}={nodestime}.")
    return int(counts[-1])


def check_node_clock() -> None:
    """`nodestime` live: a clock search repeats its node count exactly and stays under the node clock."""
    limit = NODES_PER_MS * NODE_CLOCK[0]
    for _, hce in ORACLE_ARMS:
        first, second = clock_search_nodes(hce, NODES_PER_MS), clock_search_nodes(hce, NODES_PER_MS)
        if first != second:
            raise Refused(f"DEAD WIRE - with {NODESTIME_OPTION}={NODES_PER_MS} and {HCE_OPTION}={hce} two clock "
                          f"searches count {first} and {second} nodes; a node clock repeats exactly.")
        if not 0 < first < limit:
            raise Refused(f"DEAD WIRE - a clock search with {NODESTIME_OPTION}={NODES_PER_MS} counts {first} nodes, "
                          f"outside the node clock's {limit}.")
        doubled = clock_search_nodes(hce, 2 * NODES_PER_MS)
        if doubled <= first:
            raise Refused(f"DEAD WIRE - doubling {NODESTIME_OPTION} does not raise the search's nodes "
                          f"({first} then {doubled}) with {HCE_OPTION}={hce}.")
        print(f"Node clock live: {HCE_OPTION}={hce}, {first} nodes twice at {NODESTIME_OPTION}={NODES_PER_MS}, "
              f"{doubled} at {2 * NODES_PER_MS}.")


def check_oracle(read: str) -> None:
    """The oracle: its hash, a manifest naming REVISION's src/, a live evaluation switch and, for e0n, a live node clock."""
    check_binary(ORACLE["path"], ORACLE["sha"])
    fields = manifest_fields(ORACLE["path"])
    if f"({REVISION})" not in fields.get("basilisk_src", "") or "byte-identical" not in fields.get("basilisk_src", ""):
        raise Refused(f"PROVENANCE MISMATCH - the oracle's basilisk_src is {fields.get('basilisk_src')!r}, "
                      f"not {REVISION}'s, byte-identical.")
    if fields.get("binary_sha256", "").upper() != ORACLE["sha"]:
        raise Refused(f"PROVENANCE MISMATCH - the oracle's manifest records {fields.get('binary_sha256')!r}.")
    donor, head = oracle_bench("false"), oracle_bench("true")
    if donor == head:
        raise Refused(f"DEAD WIRE - {HCE_OPTION} does not move the oracle's {WIRE_BENCH} ({donor} nodes both ways).")
    print(f"Oracle: hash and manifest pinned; {WIRE_BENCH}: {donor} nodes with {HCE_OPTION}=false, {head} with true.")
    if read == "e0n":
        check_node_clock()


def check_read(read: str) -> None:
    if read in ("e0", "e0n"):
        check_oracle(read)
    else:
        check_head()
        check_binary(SF16["path"], SF16["sha"])
        print(f"Head: hash and manifest pinned ({REVISION[:10]}, clean, bench {HEAD['bench']}); Stockfish 16 pinned.")


def params(read: str, smoke: bool) -> dict:
    common = {"Hash": 64, "Threads": 1, "MarginMs": 20, "Dir": str(run_dir(read, smoke)),
              "Concurrency": 4 if smoke else 14}
    if read in ("e0", "e0n"):
        base, increment = NODE_CLOCK if read == "e0n" else CLOCK
        engines = [(label(name, smoke), str(ORACLE["path"]), ORACLE["sha"], 1500, "Threads",
                    [f"{option}={value}" for option, value in oracle_options(read, hce).items()])
                   for name, hce in ORACLE_ARMS]
        extra = gauntlet_args(engines, SMOKE_CYCLES if smoke else CYCLES) + ["--max-engine-faults", "0"]
        out = {**common, "Mode": "gauntlet", "BaseMs": base, "IncrementMs": increment, "Seed": METER_SEED,
               "OpponentFaultRatePercent": 0, "ExpectBench": [-1, -1], "ExpectSha256": [e[2] for e in engines],
               "ThreadOptionNames": [e[4] for e in engines], "ExtraArgs": extra}
        if read == "e0n":
            out["RunFile"] = str(NODE_CLOCK_RUN_FILE)
        return out
    out = {**common, "Mode": "match", "Games": 2 * SMOKE_CYCLES if smoke else GAMES, "BaseMs": CLOCK[0],
           "IncrementMs": CLOCK[1], "Seed": FRONTIER_SEED, "ExpectRevision": REVISION,
           "EngineA": str(HEAD["path"]), "NameA": label("Basilisk head", smoke),
           "EngineB": str(SF16["path"]), "NameB": label("Stockfish 16 HCE", smoke), "OptionsB": ["Use NNUE=false"],
           "ExpectBench": [HEAD["bench"], -1], "ExpectSha256": [HEAD["sha"], SF16["sha"]]}
    if read == "odds":
        out["BaseMsB"], out["IncrementMsB"] = ODDS_CLOCK
    return out


def command(read: str, dry_run: bool, smoke: bool) -> str:
    values = params(read, smoke)
    parts = [f"& {ps(REPO / 'tools' / 'colosseum.ps1')}"]
    for key, value in values.items():
        parts.append(f"-{key} {ps(value)}")
    if values["Mode"] == "match":
        parts.append("-ExternalB")
    if dry_run:
        parts.append("-DryRun")
    return " ".join(parts) + "; exit $LASTEXITCODE"


def latest_dry_run(read: str, smoke: bool) -> pathlib.Path:
    """colosseum.ps1 writes each dry run's record beside the results, named by the run directory."""
    name = run_dir(read, smoke).name
    found = sorted(RESULTS.glob(f"colosseum_*_{name}_*.dry-run.json"), key=lambda p: p.stat().st_mtime)
    if not found:
        raise Refused(f"NO LAUNCH RECORD - colosseum wrote no dry-run record for {name} in {RESULTS}.")
    return found[-1]


def clocks_in(record) -> dict[str, tuple[int, int]]:
    """Every clock in a resolved configuration by its key (`time_control`, `engine_a_time_control`, ...)."""
    found = {}
    if isinstance(record, dict):
        for key, value in record.items():
            if key.endswith("time_control") and isinstance(value, dict):
                clock = value.get("control", {}).get("Increment")
                if not isinstance(clock, dict) or set(clock) != {"base_ms", "inc_ms"}:
                    raise Refused(f"LAUNCH RECORD - {key} is {value!r}, not a base and an increment.")
                found[key] = (clock["base_ms"], clock["inc_ms"])
            found.update(clocks_in(value))
    elif isinstance(record, list):
        for value in record:
            found.update(clocks_in(value))
    return found


def check_launch_record(read: str, smoke: bool) -> None:
    """The oracle's arms must carry exactly their options, and every clock in the record must be the read's."""
    path = latest_dry_run(read, smoke)
    resolved = json.loads(path.read_text(encoding="utf-8-sig"))["resolved_configuration"]
    if read in ("e0", "e0n"):
        want = {label(name, smoke): {**oracle_options(read, hce), "Hash": "64", "Threads": "1"}
                for name, hce in ORACLE_ARMS}
        got = {name: {k: v.lower() for k, v in options.items()} for name, options in launched_arms(resolved).items()}
        want = {name: {k: v.lower() for k, v in options.items()} for name, options in want.items()}
        if got != want:
            raise Refused(f"LAUNCH RECORD - {path.name} gives the arms {got}, not {want}.")
    clocks = clocks_in(resolved)
    if read == "odds":
        expected = {"engine_a_time_control": CLOCK, "engine_b_time_control": ODDS_CLOCK}
        wrong = clocks != expected
    else:
        expected = NODE_CLOCK if read == "e0n" else CLOCK
        wrong = not clocks or any(clock != expected for clock in clocks.values())
    if wrong:
        raise Refused(f"LAUNCH RECORD - {path.name} resolves the clocks {clocks}, not {expected}.")
    print(f"Launch record {path.name}: the options and the clocks {clocks} are the read's.")


def play(read: str, dry_run: bool, smoke: bool) -> int:
    if (run_dir(read, smoke) / "result.json").exists():
        if dry_run:
            print(f"{read}: finished; nothing to dry-run.")
            return 0
        raise Refused(f"STALE RESULT - {run_dir(read, smoke)} holds a finished read; it is not extended or replaced.")
    check_read(read)
    status = subprocess.run(powershell() + [command(read, dry_run, smoke)], cwd=REPO).returncode
    if status == 0 and dry_run:
        check_launch_record(read, smoke)
    return status


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--read", required=True, choices=ORDER + ["all"])
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--smoke", action="store_true")
    args = parser.parse_args()
    reads = ORDER if args.read == "all" else [args.read]
    try:
        for read in reads:
            if args.read == "all" and not args.dry_run and (run_dir(read, args.smoke) / "result.json").exists():
                print(f"{read}: finished; skipped.")
                continue
            print(f"== {read}{' (smoke)' if args.smoke else ''}")
            status = play(read, args.dry_run, args.smoke)
            if status != 0:
                print(f"{read} stopped with exit {status}; the run stops here.", file=sys.stderr)
                return status
    except Refused as refusal:
        print(refusal, file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
