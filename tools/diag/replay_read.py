#!/usr/bin/env python3
"""M1's outputs from the rows of `replay_games.py`.

For each column (a stop arm, or the OFF arm's reference):

- the firing rate, overall and by score band (won at 300 or more, even, lost,
  the row's own last score; a mate counts by its sign);
- the hole by score band and by where it began, with the time it took past the
  optimum (`won_battery.hole_class`: the longest root search ends after twice the
  optimum, or at the maximum, without the stop firing);
- spend, searches over 0.5x the maximum and at it, searches past the optimum;
- root best-move changes per completed iteration, and the shares of windows whose
  iteration's effort is at or over `TmEffortHi` and at or under `TmEffortLo`;
- ply-cap returns per game and their share in check.

`--removed-against` adds the time removed: a column against the stop-removed
column of the same binary, position by position over the same rows, as the sum of
the differences over the free column's spend. Each position's band is the free
row's, so one position stays in one band whichever arm is read.

`--rule` applies the stop packet's §5 rule to a column with a removed read: keep
1x under 10% of searches and 3% of spend, otherwise 2x against 1x. `--split N`
also reads the first N games against the rest (a prediction's falsifier asks it).

Exit status: 0, or 2 on bad input.

Usage:
  python tools/diag/replay_read.py --column arm-1x=a.jsonl --column arm-2x=b.jsonl --column arm-0=c.jsonl \
      --column off-1x=d.jsonl --removed-against arm-1x=arm-0 --removed-against arm-2x=arm-0 \
      --rule arm-1x --out m1.json
"""

from __future__ import annotations

import argparse
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from won_battery import AT_MAX_SHARE, OVER_SHARE, hole_class

BANDS = ("won", "even", "lost", "unscored")
KEEP_FIRED, KEEP_REMOVED = 0.10, 0.03


def band_of(score: str | None) -> str:
    if score is None:
        return "unscored"
    kind, value = score.split()
    if kind == "mate":
        return "won" if int(value) > 0 else "lost"
    return "won" if int(value) >= 300 else "lost" if int(value) <= -300 else "even"


def hole_of(row: dict) -> str | None:
    return hole_class(row["optimum"], row["maximum"], row["longest_window_start"], row["longest_window"], row["elapsed"], row["fhstop_fired"])


def blank_hole() -> dict:
    return {"before": 0, "after": 0, "other": 0, "past_optimum_ms": 0}


def ratio(top: float, bottom: float) -> float | None:
    return top / bottom if bottom else None


def read_column(rows: list[dict]) -> dict:
    bands = {band: {"searches": 0, "fired": 0, "fired_share": None, "spend_ms": 0, "hole": blank_hole(), "hole_share": None} for band in BANDS}
    whole = blank_hole()
    for row in rows:
        here = bands[band_of(row["score"])]
        here["searches"] += 1
        here["fired"] += bool(row["fhstop_fired"])
        here["spend_ms"] += row["elapsed"]
        hole = hole_of(row)
        if hole:
            for target in (here["hole"], whole):
                target[hole] += 1
                target["past_optimum_ms"] += row["elapsed"] - row["optimum"]
    for here in bands.values():
        here["fired_share"] = ratio(here["fired"], here["searches"])
        here["hole_share"] = ratio(sum(here["hole"][k] for k in ("before", "after", "other")), here["searches"])
    games = len({row["game"] for row in rows})
    cap_hits = sum(row["ply_cap_hits"] + row["qs_ply_cap_hits"] for row in rows)
    cap_in_check = sum(row["ply_cap_in_check"] + row["qs_ply_cap_in_check"] for row in rows)
    windows = sum(row["asp_windows"] for row in rows)
    return {
        "games": games,
        "searches": len(rows),
        "fired": sum(bool(row["fhstop_fired"]) for row in rows),
        "fired_share": ratio(sum(bool(row["fhstop_fired"]) for row in rows), len(rows)),
        "spend_ms": sum(row["elapsed"] for row in rows),
        "over_half": sum(row["elapsed"] >= OVER_SHARE * row["maximum"] for row in rows),
        "at_max": sum(row["elapsed"] >= AT_MAX_SHARE * row["maximum"] for row in rows),
        "past_optimum": sum(row["elapsed"] > row["optimum"] for row in rows),
        "hole": whole,
        "hole_share": ratio(sum(whole[k] for k in ("before", "after", "other")), len(rows)),
        "changes_per_iteration": ratio(sum(row["changes"] for row in rows), sum(row["iterations"] for row in rows)),
        "effort_hi_share": ratio(sum(row["root_effort_hi"] for row in rows), windows),
        "effort_lo_share": ratio(sum(row["root_effort_lo"] for row in rows), windows),
        "ply_cap_per_game": ratio(cap_hits, games),
        "ply_cap_in_check_share": ratio(cap_in_check, cap_hits),
        "bands": bands,
    }


def time_removed(rows: list[dict], free: list[dict]) -> dict:
    """What the stop takes off the free arm's clock, on the same positions."""
    here = {(row["game"], row["ply"]): row for row in rows}
    there = {(row["game"], row["ply"]): row for row in free}
    if set(here) != set(there):
        raise ValueError(f"the columns do not read the same positions: {len(set(here) ^ set(there))} differ")
    spend = sum(row["elapsed"] for row in free)
    bands = {band: {"removed_ms": 0, "free_ms": 0, "share_of_spend": None, "share_of_band": None} for band in BANDS}
    for key, row in there.items():
        band = bands[band_of(row["score"])]
        band["free_ms"] += row["elapsed"]
        band["removed_ms"] += row["elapsed"] - here[key]["elapsed"]
    for band in bands.values():
        band["share_of_spend"] = ratio(band["removed_ms"], spend)
        band["share_of_band"] = ratio(band["removed_ms"], band["free_ms"])
    removed = sum(band["removed_ms"] for band in bands.values())
    return {"removed_ms": removed, "share": ratio(removed, spend), "bands": bands}


def m1_rule(fired_share: float, removed_share: float) -> str:
    return "keep 1x" if fired_share < KEEP_FIRED and removed_share < KEEP_REMOVED else "2x against 1x"


def split_games(rows: list[dict], first: int) -> tuple[list[dict], list[dict]]:
    """The first games in the order they were replayed, and the rest."""
    order = list(dict.fromkeys(row["game"] for row in rows))
    head = set(order[:first])
    return [row for row in rows if row["game"] in head], [row for row in rows if row["game"] not in head]


def load_rows(path: pathlib.Path) -> list[dict]:
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def percent(value: float | None) -> str:
    return "-" if value is None else f"{100 * value:.2f}%"


def print_column(name: str, column: dict) -> None:
    removed = column.get("removed")
    print(f"{name}: {column['games']} games, {column['searches']} searches; stop fired {column['fired']} ({percent(column['fired_share'])}); "
          f"spent {column['spend_ms']:,} ms" + (f"; removed {removed['removed_ms']:,} ms ({percent(removed['share'])})" if removed else ""))
    print(f"   over 0.5x the maximum {column['over_half']}; at it {column['at_max']}; past the optimum {column['past_optimum']}; "
          f"changes/iteration {column['changes_per_iteration']}; effort hi {percent(column['effort_hi_share'])} lo {percent(column['effort_lo_share'])}; "
          f"ply cap/game {column['ply_cap_per_game']}, in check {percent(column['ply_cap_in_check_share'])}")
    hole = column["hole"]
    print(f"   hole {percent(column['hole_share'])}: before {hole['before']}, after {hole['after']}, other {hole['other']}; past the optimum {hole['past_optimum_ms']:,} ms")
    for band in BANDS:
        row = column["bands"][band]
        if not row["searches"]:
            continue
        line = (f"   {band}: searches {row['searches']}; fired {row['fired']} ({percent(row['fired_share'])}); spend {row['spend_ms']:,} ms; "
                f"hole {percent(row['hole_share'])} (before {row['hole']['before']}, after {row['hole']['after']}, other {row['hole']['other']}, "
                f"past the optimum {row['hole']['past_optimum_ms']:,} ms)")
        if removed:
            line += f"; removed {percent(removed['bands'][band]['share_of_spend'])} of spend, {percent(removed['bands'][band]['share_of_band'])} of the band"
        print(line)


def named(values: list[str], flag: str) -> list[tuple[str, str]]:
    pairs = [value.split("=", 1) for value in values]
    if any(len(pair) != 2 for pair in pairs):
        raise ValueError(f"{flag} takes NAME=VALUE")
    return [(pair[0], pair[1]) for pair in pairs]


def read_all(columns: dict[str, list[dict]], references: list[tuple[str, str]]) -> dict[str, dict]:
    report = {name: read_column(rows) for name, rows in columns.items()}
    for name, free in references:
        report[name]["removed"] = time_removed(columns[name], columns[free])
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--column", action="append", required=True, metavar="NAME=ROWS", help="a column and its replay's rows")
    parser.add_argument("--removed-against", action="append", default=[], metavar="NAME=FREE", help="a column and its stop-removed column")
    parser.add_argument("--rule", help="the column the stop packet's rule reads")
    parser.add_argument("--split", type=int, help="also read the first N games against the rest")
    parser.add_argument("--out", type=pathlib.Path)
    args = parser.parse_args()
    try:
        paths = named(args.column, "--column")
        references = named(args.removed_against, "--removed-against")
    except ValueError as error:
        print(error, file=sys.stderr)
        return 2
    columns = {}
    for name, path in paths:
        if not pathlib.Path(path).exists():
            print(f"no rows at {path}", file=sys.stderr)
            return 2
        columns[name] = load_rows(pathlib.Path(path))
    unknown = [name for pair in references for name in pair if name not in columns]
    if unknown or (args.rule and args.rule not in dict(references)):
        print(f"no such column, or no removed read for the rule: {unknown or args.rule}", file=sys.stderr)
        return 2
    try:
        report: dict = {"columns": read_all(columns, references)}
        if args.split:
            parts = {name: split_games(rows, args.split) for name, rows in columns.items()}
            report["split"] = {"first_games": args.split,
                               "first": read_all({name: part[0] for name, part in parts.items()}, references),
                               "rest": read_all({name: part[1] for name, part in parts.items()}, references)}
    except ValueError as error:
        print(error, file=sys.stderr)
        return 2
    for name, column in report["columns"].items():
        print_column(name, column)
    if args.split:
        for part in ("first", "rest"):
            for name, column in report["split"][part].items():
                print_column(f"{name}, {part} {args.split if part == 'first' else 'games after'}", column)
    if args.rule:
        column = report["columns"][args.rule]
        verdict = m1_rule(column["fired_share"], column["removed"]["share"])
        report["rule"] = {"column": args.rule, "fired_share": column["fired_share"], "removed_share": column["removed"]["share"], "verdict": verdict}
        print(f"the rule on {args.rule}: fired {percent(column['fired_share'])}, removed {percent(column['removed']['share'])}: {verdict}")
    if args.out:
        args.out.write_text(json.dumps(report, indent=1), encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
