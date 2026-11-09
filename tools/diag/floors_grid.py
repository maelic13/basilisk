#!/usr/bin/env python3
"""The denser conversion floor: the four families on the 25-budget grid.

Runs `test_endgames --conversion --nodes N --list` at each budget, one after another
(the harness spreads its playouts over the host's threads), names every playout
in `floors.json` and applies the registered bound, which prints as PASS or FAIL
and sets the exit status:

  KQ-K, KR-K and KBB-K: at least 598 of 600 each;
  KBN-K: at least 575 of 600, and the five registered budgets' sum at least 112.

The exit status of each run is recorded but does not decide: a run can trip the
fixture's own stale-bound guard, and the bound here replaces the per-budget read.
The wrong-bishop draws are read from one run of the harness's correctness suite,
whose exit status does count.

An interrupted grid resumes from `floors.json`, keyed by the binary's SHA-256.

Usage:
  python tools/diag/floors_grid.py --binary build/x/test_endgames.exe --out DIR
"""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import subprocess
import sys

REPO = pathlib.Path(__file__).resolve().parents[2]
GRID = (25000, 27500, 30000, 32500, 35000, 37500, 40000, 42500, 45000, 47500, 50000, 52500,
        55000, 57500, 60000, 65000, 70000, 75000, 80000, 85000, 90000, 95000, 105000, 120000,
        135000)
FIVE = (30000, 45000, 60000, 90000, 120000)
FAMILIES = ("KQ-K", "KR-K", "KBB-K", "KBN-K")
PLAYOUTS_PER_RUN = 96
PLAYOUT = re.compile(r"^playout (K\S+) (-?\d+) (.+)$")
MAJOR_FLOOR = 598
KBN_FLOOR = 575
KBN_FIVE_FLOOR = 112


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def parse_playouts(text: str) -> list[list]:
    rows = []
    for line in text.splitlines():
        match = PLAYOUT.match(line.strip())
        if match:
            rows.append([match.group(1), int(match.group(2)), match.group(3)])
    return rows


def converted_by_budget(runs: dict) -> dict[int, dict[str, int]]:
    """Converted playouts of each family at each budget."""
    table: dict[int, dict[str, int]] = {}
    for key, run in runs.items():
        counts = {family: 0 for family in FAMILIES}
        for family, plies, _ in run["playouts"]:
            if family in counts and plies >= 0:
                counts[family] += 1
        table[int(key)] = counts
    return table


def apply_bound(table: dict[int, dict[str, int]]) -> tuple[bool, list[str]]:
    """The registered bound over a whole grid: every line, then the verdict."""
    missing = [budget for budget in GRID if budget not in table]
    if missing:
        return False, [f"FAIL: budgets missing: {missing}"]
    totals = {family: sum(table[budget][family] for budget in GRID) for family in FAMILIES}
    five = sum(table[budget]["KBN-K"] for budget in FIVE)
    lines, passed = [], True
    for family in FAMILIES:
        floor = KBN_FLOOR if family == "KBN-K" else MAJOR_FLOOR
        ok = totals[family] >= floor
        passed = passed and ok
        lines.append(f"{family}: {totals[family]} of {24 * len(GRID)} (floor {floor}) "
                     f"{'pass' if ok else 'MISS'}")
    ok = five >= KBN_FIVE_FLOOR
    passed = passed and ok
    lines.append(f"KBN-K, the five budgets: {five} (floor {KBN_FIVE_FLOOR}) {'pass' if ok else 'MISS'}")
    return passed, lines


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--binary", required=True, type=pathlib.Path)
    parser.add_argument("--out", required=True, type=pathlib.Path)
    parser.add_argument("--epd", default="tests/endgames.epd")
    parser.add_argument("--budgets", default="grid",
                        help='"grid" for the registered 25, or a comma-separated list, '
                             "which applies no bound")
    args = parser.parse_args()
    budgets = GRID if args.budgets == "grid" else tuple(int(x) for x in args.budgets.split(","))

    args.out.mkdir(parents=True, exist_ok=True)
    path = args.out / "floors.json"
    identity = sha256(args.binary)
    data = {"binary": str(args.binary), "sha256": identity, "runs": {}}
    if path.exists():
        previous = json.loads(path.read_text(encoding="utf-8"))
        if previous.get("sha256") == identity:
            data = previous

    for nodes in budgets:
        key = str(nodes)
        if key in data["runs"]:
            continue
        log = args.out / f"conversion_{nodes}.log"
        with log.open("w", encoding="utf-8", errors="replace") as stream:
            code = subprocess.run([str(args.binary), args.epd, "--conversion", "--nodes", str(nodes), "--list"],
                                  cwd=REPO, stdout=stream, stderr=subprocess.STDOUT).returncode
        rows = parse_playouts(log.read_text(encoding="utf-8", errors="replace"))
        if len(rows) != PLAYOUTS_PER_RUN:
            print(f"FAIL: {nodes} nodes named {len(rows)} playouts, not {PLAYOUTS_PER_RUN} (exit {code})")
            return 2
        data["runs"][key] = {"exit": code, "playouts": rows}
        path.write_text(json.dumps(data), encoding="utf-8")
        counts = converted_by_budget({key: data["runs"][key]})[nodes]
        print(nodes, " ".join(f"{f} {counts[f]}" for f in FAMILIES), f"exit {code}", flush=True)

    log = args.out / "correctness.log"
    with log.open("w", encoding="utf-8", errors="replace") as stream:
        code = subprocess.run([str(args.binary), args.epd], cwd=REPO, stdout=stream,
                              stderr=subprocess.STDOUT).returncode
    data["correctness_exit"] = code
    path.write_text(json.dumps(data), encoding="utf-8")
    print(f"wrong-bishop draws and the correctness suite: exit {code}")

    if args.budgets != "grid":
        return 1 if code else 0
    passed, lines = apply_bound(converted_by_budget(data["runs"]))
    print("\n".join(lines))
    passed = passed and code == 0
    print("floors grid: PASS" if passed else "floors grid: FAIL")
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
