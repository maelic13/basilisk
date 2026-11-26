#!/usr/bin/env python3
"""C.3.4's results table from the finished seed reads (BAS-M25, BAS-O16).

For each finished read under tools/results/c34-<read>/ it recounts the PGN
and refuses the read when the recount disagrees with the run's standings or
holds other than 3,000 games in full pairs (c210_table.py's checks). It
reports the first arm's Elo with a 95% interval (BAS-O08's method), the
pentanomial, the first arm's wins, draws and losses, each arm's nodes a
second and nodes a move, the draws with a bare-king win on the board by the
arm that held it, and two flags: an arm outside 20% to 80% of the points,
and one arm more than 5% faster in nodes a second than the other (BAS-O16's
rule: a flagged read decides nothing alone).

  m25  BAS-M25: the new arm at its seed values minus the head
  o16  BAS-O16: the donor's evaluation minus the rebuilt one, in one search

The registered rules (below +30 for m25, above +50 for o16, the leaf returns
to research) are applied in the packet, not here.

Writes c34-table.json and c34-table.md under the results directory. Exit 1
when any finished read is refused; reads not yet played are listed as
missing.

Usage:
  python tools/diag/c34_table.py [--results tools/results]
"""

from __future__ import annotations

import argparse
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import c210_table as base  # noqa: E402
import colosseum_recount  # noqa: E402

REPO = pathlib.Path(__file__).resolve().parents[2]
ORDER = ["m25", "o16"]
ENTRY = {"m25": "BAS-M25", "o16": "BAS-O16"}
FASTER = 1.05


def read_one(directory: pathlib.Path, read: str) -> dict:
    counted = colosseum_recount.recount(directory)
    base.check_record(directory, counted)
    stats = base.pgn_stats(directory / "games.pgn")
    row = {"read": read, "entry": ENTRY[read], "first_arm": counted["engine_a"],
           "pentanomial": counted["pentanomial"], "wdl": counted["wdl"], "faults": counted["faults"],
           **base.interval(counted["pentanomial"]), **stats}
    rates = [s["nps"] for s in stats["speed"].values() if s["nps"]]
    row["saturated"] = not 0.2 <= row["score"] <= 0.8
    row["speed_ratio"] = max(rates) / min(rates) if len(rates) == 2 else None
    row["arm_faster"] = bool(row["speed_ratio"] and row["speed_ratio"] > FASTER)
    return row


def build(results: pathlib.Path) -> tuple[dict, list[str]]:
    rows, refused, missing = {}, [], []
    for read in ORDER:
        directory = results / f"c34-{read}"
        if not (directory / "result.json").exists():
            missing.append(read)
            continue
        try:
            rows[read] = read_one(directory, read)
        except base.Refused as refusal:
            refused.append(str(refusal))
    return {"schema": "basilisk-c34-table-v1", "rows": rows, "missing": missing, "refused": refused}, refused


def markdown(table: dict) -> str:
    lines = ["| Read | Entry | First arm | Elo [95%] | Pentanomial | W-D-L | Nodes/s by arm | "
             "Nodes/move by arm | Clamped draws | Flags |",
             "|---|---|---|---|---|---|---|---|---|---|"]
    for read in ORDER:
        row = table["rows"].get(read)
        if row is None:
            continue
        speed = "; ".join(f"{arm} {s['nps'] / 1e6:.2f}M" for arm, s in row["speed"].items() if s["nps"])
        if row["speed_ratio"]:
            speed += f" (ratio {row['speed_ratio']:.3f})"
        per_move = "; ".join(f"{arm} {s['nodes_per_move']:,.0f}" for arm, s in row["speed"].items()
                             if s["nodes_per_move"])
        clamped = "; ".join(f"{arm} {n}" for arm, n in row["clamped_draws"].items()) or "0"
        flags = ", ".join(flag for flag, on in (("saturated", row["saturated"]),
                                                ("an arm >5% faster", row["arm_faster"])) if on) or "-"
        lines.append(f"| {read} | {row['entry']} | {row['first_arm']} | "
                     f"{row['elo']:+.1f} [{row['low']:+.1f}, {row['high']:+.1f}] | {row['pentanomial']} | "
                     f"{'-'.join(map(str, row['wdl']))} | {speed} | {per_move} | {clamped} | {flags} |")
    if table["missing"]:
        lines.append(f"\nNot yet played: {', '.join(table['missing'])}.")
    for refusal in table["refused"]:
        lines.append(f"\nREFUSED: {refusal}")
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--results", type=pathlib.Path, default=REPO / "tools" / "results")
    args = parser.parse_args(argv)
    table, refused = build(args.results)
    (args.results / "c34-table.json").write_text(json.dumps(table, indent=2) + "\n", encoding="utf-8")
    text = markdown(table)
    (args.results / "c34-table.md").write_text(text, encoding="utf-8")
    print(text, end="")
    return 1 if refused else 0


if __name__ == "__main__":
    sys.exit(main())
