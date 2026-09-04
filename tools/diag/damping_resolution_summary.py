#!/usr/bin/env python3
"""Summarize the PLAN 6.4.a rule-50 damping / pruning-margin probe.

`damp_rule50` multiplies the evaluation by `(199 - clock) / 199` AFTER
`apply_endgame`, so it scales the mate-drive override band too. The drive
weights were chosen to clear the 243 razoring margin, and after damping several
of them do not:

    kxk king step   300 -> 224 at clock 50   (below 243)
    kxk corner step 250 -> 187 at clock 50   (below 243)
    kbnk king step  460 -> 231 at clock 99   (below 243)
    kbnk diagonal  1900 -> 954 at clock 99   (still far above)

The hypothesis is that conversion degrades with the starting clock faster than
the shortened fifty-move horizon alone explains.

Raw conversion cannot test that on its own, because raising the clock also
removes plies: at clock 80 only 20 halfmoves remain, so any position whose DTZ
exceeds that is unwinnable by arithmetic, not by evaluation. Two controls:

* **Eligibility.** Conversion is reported only over roots whose |DTZ| fits in
  the remaining budget, `100 - clock`, and additionally over the subset
  eligible at EVERY clock, so the same positions are compared throughout.
* **The family contrast, which is the real discriminator.** KQ-K, KR-K and
  KBB-K run through `kxk_score`, whose king and corner steps fall below the
  margin by clock 50. KBN-K runs through `kbnk_score`, whose diagonal step
  stays far above it at every clock. If damping erodes the gradient, the kxk
  families degrade materially more than KBN-K on the SAME eligible positions.
  If the loss is merely the shorter horizon, both degrade alike.

DTZ progress rate is the third signal and needs no eligibility control at all,
being a per-move quantity: it is the fraction of graded moves that reduce the
distance to zeroing, which is exactly what a gradient is for.
"""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path

KXK_FAMILIES = ("KQ-K", "KR-K", "KBB-K")
KBNK_FAMILIES = ("KBN-K",)
RULE50_HALFMOVES = 100


def load(result_dir: Path, clocks):
    reports = {}
    for clock in clocks:
        path = result_dir / f"clock{clock}.json"
        if not path.is_file():
            raise ValueError(f"missing arm: {path}")
        report = json.loads(path.read_text(encoding="utf-8"))
        stored = report.get("start_halfmove_clock")
        if stored != clock:
            raise ValueError(
                f"{path.name} records start_halfmove_clock={stored}, expected {clock}"
            )
        reports[clock] = report
    engines = {r["engine_sha256"] for r in reports.values()}
    if len(engines) != 1:
        raise ValueError("arms do not share one engine binary")
    nodes = {r["nodes_per_move"] for r in reports.values()}
    if len(nodes) != 1:
        raise ValueError("arms do not share one node budget")
    return reports


def rows(report, family):
    return [
        p for p in report["families"][family]["positions"]
        if p.get("theory_wdl") == 2
    ]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("result_dir", type=Path)
    parser.add_argument("--clocks", type=int, nargs="+", default=[0, 25, 50])
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    try:
        reports = load(args.result_dir, args.clocks)
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
        parser.error(str(exc))

    families = [f for f in KXK_FAMILIES + KBNK_FAMILIES
                if f in reports[args.clocks[0]]["families"]]
    tightest = max(args.clocks)

    # Positions convertible under the tightest clock's remaining budget. Every
    # arm is scored on this same set so nothing moves because of the horizon.
    common = {}
    for family in families:
        budget = RULE50_HALFMOVES - tightest
        common[family] = {
            p["id"] for p in rows(reports[args.clocks[0]], family)
            if abs(p.get("theory_dtz") or 0) <= budget
        }

    out = {"clocks": args.clocks, "families": {}, "contrast": {}}
    print("DTZ progress rate (per-move; no eligibility control needed)")
    print("%-9s %s" % ("family", "".join("%12s" % f"clock {c}" for c in args.clocks)))
    progress = {}
    for family in families:
        cells = []
        for clock in args.clocks:
            sel = rows(reports[clock], family)
            checked = sum(p["dtz_checked_moves"] for p in sel)
            made = sum(p["dtz_progress_moves"] for p in sel)
            rate = made / checked if checked else 0.0
            cells.append(rate)
        progress[family] = cells
        print("%-9s %s" % (family, "".join("%11.4f " % c for c in cells)))

    # A family with almost nothing left after the eligibility cut cannot support
    # a conversion comparison, and printing 0.0% silently would read as a
    # catastrophic result rather than an empty one. KBN-K forced this guard: its
    # median clean-win DTZ is 50, so a clock of 80 leaves exactly one eligible
    # position out of 24.
    MIN_ELIGIBLE = 10
    thin = {f: len(ids) for f, ids in common.items() if len(ids) < MIN_ELIGIBLE}
    if thin:
        print("\nWARNING: too few positions survive the eligibility cut to compare")
        print("conversion in " + ", ".join("%s (n=%d)" % kv for kv in thin.items()))
        print("Lower the tightest clock so |DTZ| <= %d admits a usable sample, or"
              % (RULE50_HALFMOVES - tightest))
        print("read DTZ progress alone for those families.")

    print("\nConversion on roots with |DTZ| <= %d, the budget remaining at"
          % (RULE50_HALFMOVES - tightest))
    print("clock %d, scored on the same set in every arm so the horizon cannot"
          % tightest)
    print("move the number")
    print("%-9s %6s %s" % ("family", "n", "".join("%12s" % f"clock {c}" for c in args.clocks)))
    conv = {}
    for family in families:
        ids = common[family]
        cells = []
        for clock in args.clocks:
            sel = [p for p in rows(reports[clock], family) if p["id"] in ids]
            cells.append(sum(p["outcome"] == "mated" for p in sel) / len(sel) if sel else 0.0)
        conv[family] = cells
        print("%-9s %6d %s" % (family, len(ids), "".join("%11.1f%% " % (100 * c) for c in cells)))
        out["families"][family] = {
            "dtz_progress": progress[family],
            "eligible_ids": len(ids),
            "conversion": cells,
        }

    # The discriminator: kxk families against KBN-K, same clocks.
    def group(names, table):
        present = [n for n in names if n in table]
        if not present:
            return None
        return [sum(table[n][i] for n in present) / len(present)
                for i in range(len(args.clocks))]

    print("\nDiscriminator - degradation from clock %d to clock %d"
          % (args.clocks[0], tightest))
    for label, names in (("kxk_score families", KXK_FAMILIES),
                         ("kbnk_score family", KBNK_FAMILIES)):
        for metric, table in (("dtz progress", progress), ("conversion", conv)):
            g = group(names, table)
            if g is None:
                continue
            drop = g[0] - g[-1]
            print("  %-20s %-14s %.4f -> %.4f   drop %+.4f"
                  % (label, metric, g[0], g[-1], -drop))
            out["contrast"].setdefault(label, {})[metric] = {
                "first": g[0], "last": g[-1], "drop": drop}

    print("\nReading: the hypothesis predicts the kxk families degrade materially")
    print("MORE than KBN-K, because their steps cross the 243 margin by clock 50")
    print("while the kbnk diagonal never does. Similar degradation in both means")
    print("the loss is the shorter horizon, and the interaction is a curiosity.")

    if args.output:
        args.output.write_text(json.dumps(out, indent=2) + "\n",
                               encoding="utf-8", newline="\n")
        print(f"\nWrote {args.output.resolve()}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
