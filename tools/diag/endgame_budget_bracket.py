#!/usr/bin/env python3
"""The endgame truth cohort across a bracket of node budgets, and its veto rule.

One budget is a guess. A verdict that appears only at a low budget is
PROVISIONAL, because the move it turns on may be a tactic a real search sees:
Basilisk rejected its leading KBNK candidate at 60,000 nodes on exactly that,
and the rejection did not reproduce at 200,000 or 600,000 (BAS-E45). The
budgets themselves come from `nodes_per_move.py`, which reads what the gate
really searches; Rarog's 60k, 200k and 600k are not inherited.

`run` plays `endgame_truth.py` unchanged at each budget over the same frozen
cohort, one single-budget report per budget, and tabulates conversion. It
refuses to tabulate arms whose cohort digests differ.

`veto` applies the rule registered before any bracket data (C.2.7.1's audit,
§4) to a baseline and a candidate measured at the same budgets:
* a new clean-win discard or a new rule-50 failure vetoes only when it appears
  at a majority of the budgets (two of three); one that does not is reported
  as provisional;
* KBN-K is judged the same way, as its multi-budget majority;
* an engine anomaly or a malformed record vetoes at any budget.

Examples:

  python tools/diag/endgame_budget_bracket.py run --engine <release.exe> \\
      --syzygy D:/chess/tablebases/syzygy3456 --budgets 40000,120000,360000 \\
      --workers 15 --out-dir tools/results/c274/head
  python tools/diag/endgame_budget_bracket.py veto \\
      --baseline-dir tools/results/c274/head --candidate-dir <dir> \\
      --budgets 40000,120000,360000
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

from endgame_compare import load_report, paired_families  # noqa: E402
from endgame_vetoes import evaluate_vetoes  # noqa: E402

RUNNER = HERE / "endgame_truth.py"
DEFAULT_COHORT = HERE / "endgame_cohort_v1.manifest.json"
MAJORITY_KINDS = ("new_clean_win_discard", "new_rule50_failure")
ANY_BUDGET_KINDS = ("engine_anomaly", "malformed_record")


def parse_budgets(spec: str) -> list[int]:
    budgets = []
    for part in spec.split(","):
        part = part.strip()
        if not part:
            continue
        value = int(part)
        if value <= 0:
            raise ValueError(f"budget must be positive, got {value}")
        budgets.append(value)
    if not budgets:
        raise ValueError("no budgets given")
    if len(set(budgets)) != len(budgets):
        raise ValueError("duplicate budgets")
    return sorted(budgets)


def report_path(directory: Path, budget: int) -> Path:
    return directory / f"truth-{budget}.json"


def load_arm(directory: Path, budgets: list[int]) -> dict[int, dict]:
    """Every budget's report, each checked to be measured at that budget."""
    reports = {}
    for budget in budgets:
        path = report_path(directory, budget)
        if not path.is_file():
            raise ValueError(f"{path}: missing")
        report = load_report(path)
        if report.get("nodes_per_move") != budget:
            raise ValueError(f"{path}: measured at {report.get('nodes_per_move')} nodes, "
                             f"not {budget}")
        reports[budget] = report
    return reports


def cohorts_agree(reports: dict[int, dict]) -> str | None:
    """Every budget must have measured the same positions, family by family."""
    digests = {b: r["_family_digests"] for b, r in reports.items()}
    first = next(iter(digests.values()))
    for budget, value in digests.items():
        if value != first:
            return f"the {budget}-node arm measured a different position set"
    return None


def conversion_table(reports: dict[int, dict]) -> dict:
    budgets = sorted(reports)
    families = list(reports[budgets[0]]["families"])
    return {
        family: {
            str(b): {"converted": reports[b]["families"][family]["converted"],
                     "theoretically_won": reports[b]["families"][family]["theoretically_won"]}
            for b in budgets
        }
        for family in families
    }


def bracket_vetoes(per_budget: dict[int, dict[str, list[str]]]) -> dict:
    """Apply the registered rule to each budget's veto lists."""
    budgets = sorted(per_budget)
    need = len(budgets) // 2 + 1
    out = {"budgets": budgets, "majority_needed": need, "vetoes": {}, "provisional": {}}
    for kind in MAJORITY_KINDS:
        seen: dict[str, list[int]] = {}
        for budget in budgets:
            for label in per_budget[budget][kind]:
                seen.setdefault(label, []).append(budget)
        out["vetoes"][kind] = {k: v for k, v in sorted(seen.items()) if len(v) >= need}
        out["provisional"][kind] = {k: v for k, v in sorted(seen.items()) if len(v) < need}
    for kind in ANY_BUDGET_KINDS:
        seen = {}
        for budget in budgets:
            for label in per_budget[budget][kind]:
                seen.setdefault(label, []).append(budget)
        out["vetoes"][kind] = dict(sorted(seen.items()))
    out["count"] = sum(len(v) for v in out["vetoes"].values())
    return out


def cmd_run(args) -> int:
    budgets = parse_budgets(args.budgets)
    args.out_dir.mkdir(parents=True, exist_ok=True)
    for budget in budgets:
        path = report_path(args.out_dir, budget)
        if args.reuse and path.is_file():
            print(f"reusing {path}", flush=True)
            continue
        cmd = [sys.executable, str(RUNNER), "--engine", str(args.engine),
               "--cohort", str(args.cohort), "--nodes", str(budget),
               "--max-plies", str(args.max_plies), "--hash", str(args.hash),
               "--workers", str(args.workers), "--output", str(path)]
        for directory in args.syzygy:
            cmd += ["--syzygy", str(directory)]
        if args.families:
            cmd += ["--families", args.families]
        if args.cohort_limit_per_family:
            cmd += ["--cohort-limit-per-family", str(args.cohort_limit_per_family)]
        print(f"\n=== {budget:,} nodes per move ===", flush=True)
        done = subprocess.run(cmd)
        if done.returncode != 0:
            raise SystemExit(f"endgame_truth.py failed at {budget} nodes "
                             f"(exit {done.returncode}); no table is written")
    reports = load_arm(args.out_dir, budgets)
    problem = cohorts_agree(reports)
    if problem:
        raise SystemExit(f"refusing to tabulate: {problem}")
    table = conversion_table(reports)
    width = max(len(f) for f in table) + 1
    print(f"\nconversion by node budget (layer 3)\n{'family':<{width}}"
          + "".join(f"{b:>14,}" for b in budgets))
    for family, row in table.items():
        print(f"{family:<{width}}" + "".join(
            f"{row[str(b)]['converted']:>8}/{row[str(b)]['theoretically_won']:<5}"
            if row[str(b)]["theoretically_won"] else f"{'n/a':>14}" for b in budgets))
    summary = {
        "schema": "basilisk-endgame-budget-bracket-v1",
        "layer": "3_conversion",
        "budgets": budgets,
        "cohort_family_sha256": reports[budgets[0]]["_family_digests"],
        "conversion": table,
    }
    (args.out_dir / "bracket.json").write_text(json.dumps(summary, indent=2) + "\n",
                                                encoding="utf-8", newline="\n")
    print("A verdict that does not survive the bracket is PROVISIONAL.")
    return 0


def cmd_veto(args) -> int:
    budgets = parse_budgets(args.budgets)
    try:
        baseline = load_arm(args.baseline_dir, budgets)
        candidate = load_arm(args.candidate_dir, budgets)
        per_budget = {b: evaluate_vetoes(paired_families(baseline[b], candidate[b]))
                      for b in budgets}
    except ValueError as exc:
        raise SystemExit(f"refused: {exc}")
    result = {"schema": "basilisk-endgame-bracket-veto-v1",
              "baseline_dir": str(args.baseline_dir.resolve()),
              "candidate_dir": str(args.candidate_dir.resolve()),
              **bracket_vetoes(per_budget)}
    verdict = "BLOCK" if result["count"] else "PASS"
    print(f"verdict: {verdict} ({result['count']} vetoes across {budgets})")
    for kind, labels in result["vetoes"].items():
        if labels:
            print(f"{kind}: " + ", ".join(f"{k} at {v}" for k, v in labels.items()))
    for kind, labels in result["provisional"].items():
        if labels:
            print(f"provisional {kind}: " + ", ".join(f"{k} at {v}" for k, v in labels.items()))
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8",
                               newline="\n")
    return 2 if result["count"] else 0


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="command", required=True)
    run = sub.add_parser("run", help="measure the cohort at each budget")
    run.add_argument("--engine", required=True, type=Path)
    run.add_argument("--syzygy", required=True, action="append", type=Path)
    run.add_argument("--cohort", type=Path, default=DEFAULT_COHORT)
    run.add_argument("--families")
    run.add_argument("--cohort-limit-per-family", type=int, default=0)
    run.add_argument("--budgets", required=True)
    run.add_argument("--max-plies", type=int, default=100)
    run.add_argument("--hash", type=int, default=16)
    run.add_argument("--workers", type=int, default=1)
    run.add_argument("--out-dir", required=True, type=Path)
    run.add_argument("--reuse", action="store_true",
                     help="keep a budget whose report already exists")
    veto = sub.add_parser("veto", help="the registered veto rule across the bracket")
    veto.add_argument("--baseline-dir", required=True, type=Path)
    veto.add_argument("--candidate-dir", required=True, type=Path)
    veto.add_argument("--budgets", required=True)
    veto.add_argument("--output", type=Path)
    args = ap.parse_args()
    try:
        return cmd_run(args) if args.command == "run" else cmd_veto(args)
    except ValueError as exc:
        ap.error(str(exc))


if __name__ == "__main__":
    sys.exit(main())
