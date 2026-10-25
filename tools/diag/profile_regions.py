#!/usr/bin/env python3
"""Attribute a sampled profile from tools/profile.ps1 to engine regions.

Each sample is an instruction address in the engine's image. llvm-symbolizer
resolves it to its full inline chain (innermost frame first) from the
binary's line tables; the sample is charged to the region of the innermost
frame whose source file belongs to the engine (src/ or Fathom). Library
frames inlined from libc++ headers are skipped outward, and a sample with no
engine frame is `runtime`; a sample outside the engine's image is `system`.

The profile is refused, not reported, when its attribution cannot be trusted:
too few samples carry an inline chain (a symbolizer or binary without inline
records charges inlined board work to its caller, the trap xperf `-symbols`
set for Rarog), too many have no function name, or an engine file is missing
from the region map. The thresholds and the map are tools/diag/profile_regions.json.
"""

from __future__ import annotations

import argparse
import collections
import json
import math
import pathlib
import shutil
import subprocess
import sys

SAMPLES_SCHEMA = "basilisk-profile-samples-v1"
REGIONS_SCHEMA = "basilisk-profile-regions-v1"
DEFAULT_MAP = pathlib.Path(__file__).with_name("profile_regions.json")


class ProfileRefused(ValueError):
    pass


def load_map(path: pathlib.Path = DEFAULT_MAP) -> dict:
    data = json.loads(path.read_text(encoding="utf-8"))
    if data.get("schema") != REGIONS_SCHEMA:
        raise ProfileRefused(f"{path} is not a {REGIONS_SCHEMA} map")
    by_file: dict[str, str] = {}
    for region, files in data["regions"]:
        for name in files:
            if name in by_file:
                raise ProfileRefused(f"{name} is mapped to both {by_file[name]} and {region}")
            by_file[name] = region
    data["by_file"] = by_file
    return data


def is_engine_file(path: str) -> bool:
    norm = path.replace("\\", "/").lower()
    return "/src/" in norm and "/include/" not in norm


def parse_symbolizer(text: str) -> dict[int, list[tuple[str, str]]]:
    """Blocks of `--print-address` output: address, then (function, file:line:col) pairs."""
    chains: dict[int, list[tuple[str, str]]] = {}
    for block in text.replace("\r\n", "\n").strip().split("\n\n"):
        lines = [line for line in block.split("\n") if line.strip()]
        if not lines or not lines[0].startswith("0x"):
            continue
        address = int(lines[0], 16)
        frames = []
        for i in range(1, len(lines) - 1, 2):
            location = lines[i + 1].strip()
            parts = location.rsplit(":", 2)
            file = parts[0] if len(parts) == 3 else location
            frames.append((lines[i].strip(), file))
        chains[address] = frames
    return chains


def run_symbolizer(exe: pathlib.Path, addresses: list[int], symbolizer: str, inlines: bool = True) -> dict:
    args = [symbolizer, f"--obj={exe}", "--print-address", "--demangle", "--functions=linkage",
            "--inlines" if inlines else "--no-inlines"]
    payload = "\n".join(hex(a) for a in addresses) + "\n"
    result = subprocess.run(args, input=payload, capture_output=True, text=True, check=False)
    if result.returncode != 0:
        raise ProfileRefused(f"llvm-symbolizer failed ({result.returncode}): {result.stderr.strip()}")
    return parse_symbolizer(result.stdout)


def wilson(count: int, total: int, z: float = 1.959964) -> tuple[float, float]:
    if total == 0:
        return 0.0, 0.0
    p = count / total
    centre = (p + z * z / (2 * total)) / (1 + z * z / total)
    half = z * math.sqrt(p * (1 - p) / total + z * z / (4 * total * total)) / (1 + z * z / total)
    return max(0.0, centre - half), min(1.0, centre + half)


def analyse(samples: dict, chains: dict[int, list[tuple[str, str]]], regions: dict) -> dict:
    by_file = regions["by_file"]
    base = int(samples["image_base"], 16)
    counts = {int(rva, 16): n for rva, n in samples["in_module"].items()}
    in_module = sum(counts.values())
    outside = int(samples.get("outside_module", 0))
    if in_module == 0:
        raise ProfileRefused("no sample landed in the engine's image")
    region_counts: collections.Counter = collections.Counter()
    inclusive: collections.Counter = collections.Counter()
    functions: collections.Counter = collections.Counter()
    unmapped: collections.Counter = collections.Counter()
    inlined = unresolved = 0
    for rva, n in counts.items():
        frames = chains.get(base + rva)
        if frames is None:
            raise ProfileRefused(f"the symbolizer returned nothing for rva {rva:#x}")
        if len(frames) >= 2:
            inlined += n
        if not frames or frames[0][0] in ("??", ""):
            unresolved += n
        engine = [(fn, f) for fn, f in frames if is_engine_file(f)]
        for fn, f in engine:
            name = pathlib.PurePosixPath(f.replace("\\", "/")).name
            if name not in by_file:
                unmapped[name] += n
        if not engine:
            region_counts["runtime"] += n
            continue
        fn, f = engine[0]
        name = pathlib.PurePosixPath(f.replace("\\", "/")).name
        region_counts[by_file.get(name, "unmapped")] += n
        functions[(fn, by_file.get(name, "unmapped"))] += n
        for region in {by_file.get(pathlib.PurePosixPath(x.replace("\\", "/")).name, "unmapped") for _, x in engine}:
            inclusive[region] += n
    if unmapped:
        files = ", ".join(f"{k} ({v})" for k, v in unmapped.most_common())
        raise ProfileRefused(f"engine files missing from the region map: {files}")
    inline_share = inlined / in_module
    if inline_share < regions["min_inline_share"]:
        raise ProfileRefused(
            f"only {inline_share:.1%} of samples carry an inline chain (minimum "
            f"{regions['min_inline_share']:.0%}): the binary or symbolizer lacks inline records, and "
            "inlined work would be charged to its caller")
    unresolved_share = unresolved / in_module
    if unresolved_share > regions["max_unresolved_share"]:
        raise ProfileRefused(
            f"{unresolved_share:.1%} of samples have no function name (maximum "
            f"{regions['max_unresolved_share']:.0%}): is this a Profile flavor build?")
    total = in_module + outside
    if outside:
        region_counts["system"] += outside
    shares = {}
    for region, n in region_counts.most_common():
        lo, hi = wilson(n, total)
        shares[region] = {"samples": n, "share": n / total, "interval": [lo, hi],
                          "inline_inclusive": inclusive.get(region, 0) / total}
    return {
        "label": samples.get("label", ""),
        "cohort": samples.get("cohort", ""),
        "exe_sha256": samples.get("sha256", ""),
        "samples": total,
        "in_module": in_module,
        "inline_share": inline_share,
        "unresolved_share": unresolved_share,
        "regions": shares,
        "top_functions": [{"function": fn, "region": region, "share": n / total}
                          for (fn, region), n in functions.most_common(15)],
    }


def render(report: dict) -> str:
    lines = [f"Profile{' [' + report['label'] + ']' if report['label'] else ''}: cohort "
             f"{report['cohort']}, {report['samples']:,} samples; inline chains on "
             f"{report['inline_share']:.1%}, unresolved {report['unresolved_share']:.2%}"]
    lines.append("  region        self   [95%]            inline-inclusive")
    for region, data in report["regions"].items():
        lo, hi = data["interval"]
        lines.append(f"  {region:<11} {data['share']:6.1%}  [{lo:5.1%}, {hi:5.1%}]   {data['inline_inclusive']:6.1%}")
    lines.append("  top innermost engine functions:")
    for entry in report["top_functions"]:
        lines.append(f"    {entry['share']:6.2%}  {entry['region']:<10} {entry['function'][:90]}")
    return "\n".join(lines)


def find_symbolizer(explicit: str | None) -> str:
    for candidate in (explicit, shutil.which("llvm-symbolizer"), "C:/msys64/clang64/bin/llvm-symbolizer.exe"):
        if candidate and pathlib.Path(candidate).exists() or (candidate and shutil.which(candidate)):
            return candidate
    raise ProfileRefused("llvm-symbolizer not found; pass --symbolizer")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--samples", required=True, type=pathlib.Path)
    parser.add_argument("--map", type=pathlib.Path, default=DEFAULT_MAP)
    parser.add_argument("--symbolizer")
    parser.add_argument("--no-inlines", action="store_true",
                        help="symbolize without inline chains; the refusal this triggers is the check's wire")
    parser.add_argument("--json", type=pathlib.Path)
    args = parser.parse_args(argv)
    try:
        samples = json.loads(args.samples.read_text(encoding="utf-8"))
        if samples.get("schema") != SAMPLES_SCHEMA:
            raise ProfileRefused(f"{args.samples} is not a {SAMPLES_SCHEMA} file")
        exe = pathlib.Path(samples["exe"])
        if not exe.is_absolute():
            exe = args.samples.parent / exe
        base = int(samples["image_base"], 16)
        addresses = sorted(base + int(rva, 16) for rva in samples["in_module"])
        chains = run_symbolizer(exe, addresses, find_symbolizer(args.symbolizer), inlines=not args.no_inlines)
        report = analyse(samples, chains, load_map(args.map))
    except ProfileRefused as error:
        print(f"PROFILE REFUSED - {error}", file=sys.stderr)
        return 2
    print(render(report))
    if args.json:
        args.json.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
