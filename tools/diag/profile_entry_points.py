#!/usr/bin/env python3
"""Charge a profile's board samples to the board entry point the search called.

Reads a `tools/profile.ps1` samples file, symbolizes it with inline chains
(tools/diag/profile_regions.py), and charges every sample whose engine chain
holds a board frame (the region map's `board` files) to the outermost such
frame: the board function its caller outside the board called. Attack lookups
inside it are charged with it; a lookup the search or the evaluation makes
directly is not board work. Inside each entry point the sample goes to its
innermost engine function, `own` when that is the entry point itself.

    python tools/diag/profile_entry_points.py --samples tools/results/x/samples.json \\
        [--json out.json] [--top 6]

Shares are of all samples, as profile_regions.py reports them, with Wilson
95% intervals.
"""

from __future__ import annotations

import argparse
import collections
import json
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import profile_regions as pr  # noqa: E402  (symbolizer, region map, interval)


def short(function: str) -> str:
    """`Board::see_ge(unsigned short, int) const` -> `see_ge`."""
    name = function.split("(")[0]
    return name.rsplit("::", 1)[-1] or function


def attribute(samples: dict, chains: dict, regions: dict) -> dict:
    by_file = regions["by_file"]
    base = int(samples["image_base"], 16)
    total = sum(samples["in_module"].values()) + int(samples.get("outside_module", 0))
    entry: collections.Counter = collections.Counter()
    inside: dict[str, collections.Counter] = collections.defaultdict(collections.Counter)
    board_total = 0
    for rva, n in samples["in_module"].items():
        frames = chains.get(base + int(rva, 16))
        if frames is None:
            raise pr.ProfileRefused(f"the symbolizer returned nothing for rva {rva}")
        engine = [(fn, f) for fn, f in frames if pr.is_engine_file(f)]
        def region(f: str) -> str:
            return by_file.get(pathlib.PurePosixPath(f.replace("\\", "/")).name, "unmapped")
        board = [fn for fn, f in engine if region(f) == "board"]
        if not board:
            continue
        board_total += n
        outer = short(board[-1])
        inner = short(engine[0][0])
        entry[outer] += n
        inside[outer]["own" if inner == outer else inner] += n
    rows = []
    for name, n in entry.most_common():
        lo, hi = pr.wilson(n, total)
        rows.append({"entry_point": name, "samples": n, "share": n / total, "interval": [lo, hi],
                     "inside": {k: v / total for k, v in inside[name].most_common()}})
    return {"cohort": samples.get("cohort", ""), "exe_sha256": samples.get("sha256", ""),
            "revision": samples.get("revision", ""), "samples": total,
            "board_share": board_total / total, "entry_points": rows}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--samples", required=True, type=pathlib.Path)
    parser.add_argument("--map", type=pathlib.Path, default=pr.DEFAULT_MAP)
    parser.add_argument("--symbolizer")
    parser.add_argument("--json", type=pathlib.Path)
    parser.add_argument("--top", type=int, default=6, help="inner functions listed per entry point")
    args = parser.parse_args(argv)
    try:
        samples = json.loads(args.samples.read_text(encoding="utf-8"))
        if samples.get("schema") != pr.SAMPLES_SCHEMA:
            raise pr.ProfileRefused(f"{args.samples} is not a {pr.SAMPLES_SCHEMA} file")
        exe = pathlib.Path(samples["exe"])
        if not exe.is_absolute():
            exe = args.samples.parent / exe
        if not exe.is_file():
            raise pr.ProfileRefused(f"the profiled binary {exe} is gone; symbols come from it")
        base = int(samples["image_base"], 16)
        addresses = sorted(base + int(rva, 16) for rva in samples["in_module"])
        chains = pr.run_symbolizer(exe, addresses, pr.find_symbolizer(args.symbolizer))
        regions = pr.load_map(args.map)
        pr.analyse(samples, chains, regions)  # the region tool's refusals apply here too
        report = attribute(samples, chains, regions)
    except pr.ProfileRefused as error:
        print(f"PROFILE REFUSED - {error}", file=sys.stderr)
        return 2
    print(f"{report['cohort']}, {report['samples']:,} samples; board entry points "
          f"{report['board_share']:.2%} of all samples")
    for row in report["entry_points"]:
        lo, hi = row["interval"]
        parts = list(row["inside"].items())[:args.top]
        detail = "; ".join(f"{k} {100 * v:.2f}" for k, v in parts)
        print(f"  {100 * row['share']:6.2f}%  [{100 * lo:5.2f}, {100 * hi:5.2f}]  "
              f"{row['entry_point']:<28} {detail}")
    if args.json:
        args.json.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
