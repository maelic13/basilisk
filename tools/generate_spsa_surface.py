"""Generate SPSA inputs from the search-parameter X-macro.

The C++ table owns option names, defaults, and bounds.  This generator derives
the registered integer perturbation as max(2, round((max - min) / 16)) and
writes both weather-factory JSON and Colosseum TOML, for the root table and the
line the kernel inherited and, separately, for each cluster's coordinates.
Generated files are checked into the tree so experiment registrations can name
an exact surface; edit the X-macro and regenerate rather than editing them.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

GAMMA = 0.102
DEFAULT_ITERATIONS = 2_000
ROOT = pathlib.Path(__file__).resolve().parents[1]
HEADER = ROOT / "src" / "search_params.h"
CONFIG = ROOT / "tools" / "spsa_configs" / "config_search.json"
COLOSSEUM = ROOT / "tools" / "spsa_configs" / "colosseum"
TUNE = COLOSSEUM / "search.tune.toml"
RUN = COLOSSEUM / "search.run.toml"
# Cluster 1's surface: the coordinates written in its kernel table, without
# the legacy line it splices in. Its tune runs on the shared run file with
# --tune pointing here.
CORE_CONFIG = ROOT / "tools" / "spsa_configs" / "config_core.json"
CORE_TUNE = COLOSSEUM / "core.tune.toml"
# Cluster 2's surface: its table's X lines alone, on the same run file.
# Neither the default nor the core surface carries them.
B3_CONFIG = ROOT / "tools" / "spsa_configs" / "config_b3.json"
B3_TUNE = COLOSSEUM / "b3.tune.toml"
# Cluster 3's surface: its table's X lines alone, on the same run file.
# B.4.6 admits the earlier clusters' coordinates to its fit.
B4_CONFIG = ROOT / "tools" / "spsa_configs" / "config_b4.json"
B4_TUNE = COLOSSEUM / "b4.tune.toml"
# Cluster 4's surface: its table's X lines alone, on the same run file.
B5_CONFIG = ROOT / "tools" / "spsa_configs" / "config_b5.json"
B5_TUNE = COLOSSEUM / "b5.tune.toml"
# B.3.3's curvature-sweep registrations on the B3 surface (packet
# b30_cluster2_research_2026-10-30, amendment 3 section 4). A sweep coordinate is
# an F line the sweep reads at the listed points and SPSA never fits; an extra
# point is a value of an X coordinate the sweep reads beside its default.
# Both are rendered as notes in the B3 tune file and checked against the table.
B3_SWEEP_COORDINATES = {
    "B3SeekMateDepth": (8, 12, 16),
    "B3SingNegative": (0, 1, 3),
}
B3_SWEEP_EXTRA_POINTS = {
    "B3SingMargin": (17, 126),
}
# B.3.3's fit surface (BAS-S42): the B3 table's X lines less the inert ones,
# with the Cluster 1 coordinates whose consumer populations Cluster 2 moved
# (packet section 5.12, read by B.3.2 in BAS-S41), on the same run file.
B33_CONFIG = ROOT / "tools" / "spsa_configs" / "config_b33.json"
B33_TUNE = COLOSSEUM / "b33.tune.toml"
# Inert while its switch B3ProbCutTt stays 0 (BAS-S41: +0.9 +- 9.5).
B33_EXCLUDED = ("B3ProbCutTtMargin",)
B33_ADMITTED_CORE = (
    "CoreLmpBase", "CoreQuietFutBase", "CoreCaptFutBase", "CoreCaptSeeCoeff",
    "CoreContHistPruneSlope", "CoreRfpCorrMult", "CoreCorrUpdateScale",
    "CoreRfpPerPly", "CoreRfpImproving", "CoreRfpDepthCap",
)
# B.4.6's fit surface (BAS-S51): the B4 table's X lines with packet section 5.8's
# coordinates (its unconditional list, then the two B.4.5's read admitted), on
# the same run file.
B46_CONFIG = ROOT / "tools" / "spsa_configs" / "config_b46.json"
B46_TUNE = COLOSSEUM / "b46.tune.toml"
B46_ADMITTED = (
    "CoreRazorMargin", "CoreRfpPerPly", "CoreRfpImproving", "CoreRfpWorsening",
    "CoreRfpCorrMult", "CoreRfpBetaWeight", "CoreRfpDepthCap", "B3NmpBase",
    "B3IirMinDepth", "CoreLmrCutNodeNoTt", "B3NmpPerPly", "CoreResearchDeeper",
)
PARAM_RE = re.compile(
    r"^\s*X\(\s*[a-z][a-z0-9_]*\s*,\s*([A-Za-z][A-Za-z0-9]*)\s*,"
    r"\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\)"
)
LINE_RE = re.compile(
    r"^\s*([XFS])\(\s*[a-z][a-z0-9_]*\s*,\s*([A-Za-z][A-Za-z0-9]*)\s*,"
    r"\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*(-?\d+)\s*\)"
)
# Cluster 1's kernel table. The default surface (the root table and the
# legacy line) skips its lines; the core surface is its X lines alone. Its F lines are coordinates the engine reads and a tune build
# advertises but no tune fits, so, like S lines, they are never on a surface
# (only X lines match PARAM_RE).
CORE_TABLE = "#define BASILISK_CORE_KERNEL_PARAMS("
B3_TABLE = "#define BASILISK_B3_PARAMS("
B4_TABLE = "#define BASILISK_B4_PARAMS("
B5_TABLE = "#define BASILISK_B5_PARAMS("
# Cluster 2's refit of core coordinates: A(field, UciName, value) lines. The
# core surface keeps the X lines' values (Cluster 1's fit); a surface tuned
# after Cluster 2 starts from these.
B3_CORE_DEFAULTS_TABLE = "#define BASILISK_B3_CORE_DEFAULTS("
# Cluster 3's refit of earlier clusters' lines, in the same form; a surface
# tuned after Cluster 3 starts from these, over the B3 table's.
B4_CORE_DEFAULTS_TABLE = "#define BASILISK_B4_CORE_DEFAULTS("
TABLES = (CORE_TABLE, B3_TABLE, B4_TABLE, B5_TABLE, B3_CORE_DEFAULTS_TABLE,
          B4_CORE_DEFAULTS_TABLE)
ARM_DEFAULT_RE = re.compile(
    r"^\s*A\(\s*[a-z][a-z0-9_]*\s*,\s*([A-Za-z][A-Za-z0-9]*)\s*,\s*(-?\d+)\s*\)"
)
SWITCH_RE = re.compile(r"^\s*S\(\s*[a-z][a-z0-9_]*\s*,\s*([A-Za-z][A-Za-z0-9]*)\s*,", re.M)


def split_tables(text: str) -> dict[str, list[str]]:
    """The header's lines by table: each arm table's own, and the rest."""
    out: dict[str, list[str]] = {"default": [], **{table: [] for table in TABLES}}
    current = None
    for line in text.splitlines():
        if current is None:
            current = next((t for t in TABLES if line.startswith(t)), None)
        out[current or "default"].append(line)
        if current is not None and not line.rstrip().endswith("\\"):
            current = None
    return out


def split_arm_lines(text: str) -> tuple[list[str], list[str]]:
    """The header's lines outside the arm tables, and the core table's own."""
    tables = split_tables(text)
    return tables["default"], tables[CORE_TABLE]


def b3_arm_lines(text: str) -> list[str]:
    return split_tables(text)[B3_TABLE]


def b4_arm_lines(text: str) -> list[str]:
    return split_tables(text)[B4_TABLE]


def b5_arm_lines(text: str) -> list[str]:
    return split_tables(text)[B5_TABLE]


def default_arm_lines(text: str) -> list[str]:
    return split_arm_lines(text)[0]


def core_arm_lines(text: str) -> list[str]:
    return split_arm_lines(text)[1]


def parse_parameters(text: str, core: bool = False, b3: bool = False,
                     b4: bool = False, b5: bool = False) -> list[tuple[str, int, int, int]]:
    parameters = []
    seen = set()
    lines = (b5_arm_lines(text) if b5 else b4_arm_lines(text) if b4 else b3_arm_lines(text) if b3
             else core_arm_lines(text) if core else default_arm_lines(text))
    for line in lines:
        match = PARAM_RE.match(line)
        if not match:
            continue
        name, default, minimum, maximum = match.groups()
        values = tuple(map(int, (default, minimum, maximum)))
        if name in seen:
            raise ValueError(f"duplicate UCI option {name}")
        if not values[1] <= values[0] <= values[2]:
            raise ValueError(f"{name}: default {values[0]} outside [{values[1]}, {values[2]}]")
        if values[1] == values[2]:
            raise ValueError(f"{name}: empty range")
        seen.add(name)
        parameters.append((name, *values))
    if not parameters:
        raise ValueError("no BASILISK_SEARCH_PARAMS entries found")
    return parameters


def b3_core_defaults(text: str) -> dict[str, int]:
    """Cluster 2's refit of core defaults, each checked: a core X line's
    name, inside its range, off its default, named once."""
    core = {name: (default, minimum, maximum)
            for name, default, minimum, maximum in parse_parameters(text, core=True)}
    overrides: dict[str, int] = {}
    for line in split_tables(text)[B3_CORE_DEFAULTS_TABLE]:
        match = ARM_DEFAULT_RE.match(line)
        if not match:
            continue
        name, value = match.group(1), int(match.group(2))
        if name not in core:
            raise ValueError(f"Cluster 2 default {name} names no core X line")
        default, minimum, maximum = core[name]
        if not minimum <= value <= maximum or value == default or name in overrides:
            raise ValueError(f"Cluster 2 default {name} {value}: must lie in "
                             f"[{minimum}, {maximum}] off {default}, once")
        overrides[name] = value
    return overrides


def b4_core_defaults(text: str) -> dict[str, int]:
    """Cluster 3's refit of earlier clusters' X lines, each
    checked: inside its range, off its line's default, named once. A switch
    is never on a surface, so its line here is passed over."""
    lines = {name: (default, minimum, maximum)
             for name, default, minimum, maximum in
             [*parse_parameters(text, core=True), *parse_parameters(text, b3=True)]}
    switches = set(SWITCH_RE.findall(text))
    overrides: dict[str, int] = {}
    for line in split_tables(text)[B4_CORE_DEFAULTS_TABLE]:
        match = ARM_DEFAULT_RE.match(line)
        if not match:
            continue
        name, value = match.group(1), int(match.group(2))
        if name in switches:
            continue
        if name not in lines:
            raise ValueError(f"Cluster 3 default {name} names no X line")
        default, minimum, maximum = lines[name]
        if not minimum <= value <= maximum or value == default or name in overrides:
            raise ValueError(f"Cluster 3 default {name} {value}: must lie in "
                             f"[{minimum}, {maximum}] off {default}, once")
        overrides[name] = value
    return overrides


def sweep_notes(text: str, coordinates: dict[str, tuple[int, ...]],
                extra_points: dict[str, tuple[int, ...]]) -> list[str]:
    """The B3 table's sweep registrations as comment lines, each checked: a
    sweep coordinate is an F line whose points lie in its range and include
    its default; an extra point lies in its X line's range off the default."""
    table = {}
    for line in b3_arm_lines(text):
        match = LINE_RE.match(line)
        if match:
            kind, name, *values = match.groups()
            table[name] = (kind, *map(int, values))
    notes = []
    for name, points in coordinates.items():
        kind, default, minimum, maximum = table.get(name, ("?", 0, 0, 0))
        if kind != "F":
            raise ValueError(f"sweep coordinate {name} is not an F line of the B3 table")
        if default not in points or not all(minimum <= p <= maximum for p in points):
            raise ValueError(f"sweep coordinate {name}: points {points} must include "
                             f"{default} and lie in [{minimum}, {maximum}]")
        notes.append(f"# sweep coordinate {name}: {', '.join(map(str, points))} "
                     f"(default {default}; never fitted by SPSA)")
    for name, points in extra_points.items():
        kind, default, minimum, maximum = table.get(name, ("?", 0, 0, 0))
        if kind != "X":
            raise ValueError(f"sweep extra point {name} is not an X line of the B3 table")
        if default in points or not all(minimum <= p <= maximum for p in points):
            raise ValueError(f"sweep extra points {name}: {points} must lie in "
                             f"[{minimum}, {maximum}] off the default {default}")
        notes.append(f"# sweep extra points {name}: {', '.join(map(str, points))} "
                     f"(beside the default {default})")
    return notes


def b33_parameters(text: str) -> list[tuple[str, int, int, int]]:
    """B.3.3's fit surface: the B3 X lines less B33_EXCLUDED, then the core X
    lines B33_ADMITTED_CORE names, in that order; every name is checked. The
    surface is Cluster 2's, so a core coordinate starts from its refit where
    it has one."""
    b3 = parse_parameters(text, b3=True)
    overrides = b3_core_defaults(text)
    core = {name: (name, overrides.get(name, default), minimum, maximum)
            for name, default, minimum, maximum in parse_parameters(text, core=True)}
    missing = [name for name in B33_EXCLUDED if name not in {p[0] for p in b3}]
    missing += [name for name in B33_ADMITTED_CORE if name not in core]
    if missing:
        raise ValueError(f"B.3.3 surface names no X line: {', '.join(missing)}")
    return [p for p in b3 if p[0] not in B33_EXCLUDED] + [core[name] for name in B33_ADMITTED_CORE]


def b46_parameters(text: str) -> list[tuple[str, int, int, int]]:
    """B.4.6's fit surface: the B4 X lines, then the coordinates B46_ADMITTED
    names from the core and B3 tables, in that order; every name is checked.
    Each starts from the value the engine runs: Cluster 3's refit where it has
    one, then Cluster 2's."""
    overrides = {**b3_core_defaults(text), **b4_core_defaults(text)}
    earlier = {name: (name, overrides.get(name, default), minimum, maximum)
               for name, default, minimum, maximum in
               [*parse_parameters(text, core=True), *parse_parameters(text, b3=True)]}
    missing = [name for name in B46_ADMITTED if name not in earlier]
    if missing:
        raise ValueError(f"B.4.6 surface names no X line: {', '.join(missing)}")
    return parse_parameters(text, b4=True) + [earlier[name] for name in B46_ADMITTED]


def perturbation(minimum: int, maximum: int) -> int:
    # Positive integer half-up rounding; unlike Python round(), ties do not
    # depend on parity.
    return max(2, (maximum - minimum + 8) // 16)


# Coordinates whose step is at most half their start value, rounded half up
# (B.5 packet, research amendment 2, section A2.5): a sixteenth of the window
# base's range is larger than the seed's distance to its lower bound, so the
# low perturbation would clamp from the first iterate.
HALF_START_STEP = frozenset({"B5AspBase"})


def step_of(name: str, start: int, minimum: int, maximum: int) -> int:
    step = perturbation(minimum, maximum)
    if name in HALF_START_STEP:
        step = max(2, min(step, (start + 1) // 2))
    return step


def render(parameters: list[tuple[str, int, int, int]], iterations: int,
           config: pathlib.Path = CONFIG, tune: pathlib.Path = TUNE,
           run: pathlib.Path | None = RUN,
           notes: list[str] | None = None) -> dict[pathlib.Path, str]:
    surface = {
        name: {
            "value": default,
            "min_value": minimum,
            "max_value": maximum,
            "step": step_of(name, default, minimum, maximum),
        }
        for name, default, minimum, maximum in parameters
    }
    decay = iterations**-GAMMA
    tune_lines = [
        "# Generated by tools/generate_spsa_surface.py from src/search_params.h.",
        f"# N = {iterations}; c_end = step * N ** -{GAMMA}.",
        "# Regenerate rather than edit; --check rejects drift.",
        *(notes or []),
    ]
    for name, spec in surface.items():
        tune_lines.extend(
            [
                "",
                "[[parameters]]",
                f'name = "{name}"',
                f"initial = {spec['value']}",
                f"min = {spec['min_value']}",
                f"max = {spec['max_value']}",
                f"c_end = {spec['step'] * decay:.4f}",
            ]
        )
    run_text = """# Generated by tools/generate_spsa_surface.py.
# Supply the engine, horizon, seed and evidence directory through the wrapper.

extend = "../../colosseum/spsa-tune.toml"

[options]
tune = "search.tune.toml"
option = ["Hash=64", "Threads=1"]
"""
    outputs = {
        config: json.dumps(surface, indent=4) + "\n",
        tune: "\n".join(tune_lines) + "\n",
    }
    if run is not None:
        outputs[run] = run_text
    return outputs


def render_all(text: str, iterations: int) -> dict[pathlib.Path, str]:
    """The default surface and run file, then the core, B3, B.3.3, B4, B.4.6
    and B5 surfaces."""
    outputs = render(parse_parameters(text), iterations)
    outputs.update(render(parse_parameters(text, core=True), iterations,
                          config=CORE_CONFIG, tune=CORE_TUNE, run=None))
    outputs.update(render(parse_parameters(text, b3=True), iterations,
                          config=B3_CONFIG, tune=B3_TUNE, run=None,
                          notes=sweep_notes(text, B3_SWEEP_COORDINATES,
                                            B3_SWEEP_EXTRA_POINTS)))
    outputs.update(render(b33_parameters(text), iterations,
                          config=B33_CONFIG, tune=B33_TUNE, run=None,
                          notes=["# B.3.3's fit surface (BAS-S42): the B3 X lines less "
                                 + ", ".join(B33_EXCLUDED) + ", with section 5.12's "
                                 "Cluster 1 admissions."]))
    outputs.update(render(parse_parameters(text, b4=True), iterations,
                          config=B4_CONFIG, tune=B4_TUNE, run=None))
    outputs.update(render(b46_parameters(text), iterations,
                          config=B46_CONFIG, tune=B46_TUNE, run=None,
                          notes=["# B.4.6's fit surface (BAS-S51): the B4 X lines with "
                                 "section 5.8's coordinates."]))
    outputs.update(render(parse_parameters(text, b5=True), iterations,
                          config=B5_CONFIG, tune=B5_TUNE, run=None))
    return outputs


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iterations", type=int, default=DEFAULT_ITERATIONS)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.iterations < 1:
        parser.error("--iterations must be positive")
    try:
        text = HEADER.read_text(encoding="utf-8")
        outputs = render_all(text, args.iterations)
    except ValueError as error:
        print(error, file=sys.stderr)
        return 2
    if args.check:
        stale = [
            path
            for path, wanted in outputs.items()
            if not path.is_file()
            or path.read_text(encoding="utf-8").replace("\r\n", "\n") != wanted
        ]
        for path in stale:
            print(f"stale or missing: {path}", file=sys.stderr)
        if stale:
            return 1
        print(f"{len(outputs)} generated files current ({len(parse_parameters(text))} default, "
              f"{len(parse_parameters(text, core=True))} core, "
              f"{len(parse_parameters(text, b3=True))} B3, "
              f"{len(b33_parameters(text))} B.3.3 fit-surface, "
              f"{len(parse_parameters(text, b4=True))} B4 and "
              f"{len(b46_parameters(text))} B.4.6 fit-surface parameters)")
        return 0
    COLOSSEUM.mkdir(parents=True, exist_ok=True)
    for path, content in outputs.items():
        path.write_text(content, encoding="utf-8", newline="")
        print(f"wrote {path.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
