"""Compare a Colosseum dry run with a recorded `sprt.ps1` manifest."""

from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import sys

LINE = re.compile(r"^(?P<key>[A-Za-z0-9_]+):\s*(?P<value>.*)$")
DESIGN = re.compile(
    r"SPRT\s+elo0=(?P<elo0>-?[\d.]+)\s+elo1=(?P<elo1>-?[\d.]+)\s+"
    r"alpha=(?P<alpha>[\d.]+)\s+beta=(?P<beta>[\d.]+)\s+model=(?P<model>\w+)"
)
CLOCK = re.compile(r"tc=(?P<base>[\d.]+)\+(?P<inc>[\d.]+)")
MARGIN = re.compile(r"timemargin=(?P<margin>\d+)ms")


def read_manifest(path: pathlib.Path) -> dict[str, str]:
    fields: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        match = LINE.match(line.strip())
        if match:
            fields[match.group("key")] = match.group("value").strip()
    if "engineA" not in fields:
        raise SystemExit(f"{path} is not an sprt.ps1 manifest")
    return fields


def sha256(path: pathlib.Path) -> str | None:
    if not path.is_file():
        return None
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def slot_physical_cpus(resolved: dict) -> list[int]:
    cpus: set[int] = set()
    for slot in resolved.get("execution", {}).get("slots", []):
        for side in ("engine_a", "engine_b"):
            for cpu in slot.get(side, {}).get("allocation", {}).get("cpus", []) or []:
                cpus.add(int(cpu["number"]))
    return sorted(cpu for cpu in cpus if cpu % 2 == 0)


def engine_options(engine: dict) -> dict[str, str]:
    return {name: str(spec["value"]) for name, spec in (engine.get("options") or {}).items()}


def compare(manifest: dict[str, str], dry: dict) -> list[dict]:
    resolved = dry["resolved_configuration"]
    rows: list[dict] = []

    def check(field: str, fastchess, colosseum, ok: bool | None = None, note: str = "") -> None:
        rows.append({"field": field, "fastchess": fastchess, "colosseum": colosseum,
                     "equal": fastchess == colosseum if ok is None else ok, "note": note})

    design = DESIGN.search(manifest.get("test_design", ""))
    parameters = resolved.get("design", {}).get("parameters", {})
    if design:
        check("sprt.model", design.group("model"), str(parameters.get("model")))
        for bound in ("elo0", "elo1", "alpha", "beta"):
            check(f"sprt.{bound}", float(design.group(bound)), float(parameters.get(bound, "nan")))
        check("game_budget", int(manifest["game_budget"]),
              int(resolved.get("design", {}).get("max_pairs", 0)) * 2)
    elif manifest.get("test_design", "").startswith("fixed") and resolved.get("command") == "match":
        check("match.design", "fixed", "fixed")
        check("game_budget", int(manifest["game_budget"]), int(resolved.get("games", 0)))
    else:
        check("design", manifest.get("test_design"), resolved.get("command"), ok=False)

    clock = CLOCK.search(manifest.get("time_control", ""))
    margin = MARGIN.search(manifest.get("time_control", ""))
    for name in ("engine_a_time_control", "engine_b_time_control"):
        control = resolved.get(name, {})
        increment = control.get("control", {}).get("Increment", {})
        check(f"{name}.base_ms", round(float(clock.group("base")) * 1000), int(increment.get("base_ms", -1)))
        check(f"{name}.inc_ms", round(float(clock.group("inc")) * 1000), int(increment.get("inc_ms", -1)))
        check(f"{name}.margin_ms", int(margin.group("margin")), int(control.get("margin_ms", -1)))

    adjudication = resolved.get("adjudication", {})
    fc_adjudicated = not manifest.get("adjudication", "").lower().startswith("none")
    col_adjudicated = any(adjudication.get(rule) is not None for rule in ("draw", "resign", "max_moves"))
    check("adjudication", fc_adjudicated, col_adjudicated)

    for manifest_key, resolved_key, option_key in (("engineA", "engine_a", "optionsA"),
                                                    ("engineB", "engine_b", "optionsB")):
        wanted = manifest.get(manifest_key, "")
        _, _, raw_path = wanted.partition("=")
        manifest_path = pathlib.Path(raw_path.strip() or wanted)
        col_path = pathlib.Path(resolved[resolved_key]["executable"])
        check(f"{manifest_key}.executable", str(manifest_path).lower(), str(col_path).lower())
        check(f"{manifest_key}.sha256", manifest[f"{manifest_key}_sha256"].upper(),
              sha256(col_path) or "(binary missing)")
        options = engine_options(resolved[resolved_key])
        check(f"{manifest_key}.Hash", manifest["hash_mb"], options.get("Hash"))
        check(f"{manifest_key}.Threads", manifest["threads"], options.get("Threads"))
        actual_extra = sorted(set(options) - {"Hash", "Threads"})
        raw_extra = manifest.get(option_key, "(none)")
        wanted_extra = [] if raw_extra == "(none)" else sorted(item.split("=", 1)[0] for item in raw_extra.split())
        check(f"{manifest_key}.extra_options", wanted_extra, actual_extra)

    openings = resolved.get("openings", {})
    book = pathlib.Path(manifest["book"])
    resolved_book = pathlib.Path(openings.get("path", ""))
    check("book", str(book).lower(), str(resolved_book).lower())
    check("book_sha256", manifest["book_sha256"].upper(), sha256(resolved_book) or "(book missing)")
    check("opening_order", manifest["opening_order"].lower(), str(openings.get("order", "")).lower())
    check("opening_seed", int(manifest["opening_seed"]), int(resolved.get("master_seed", -1)))
    check("concurrency", int(manifest["concurrency"]), int(resolved.get("execution", {}).get("concurrency", -1)))
    cpu_prefix = manifest["affinity_cpus"].split(" ", 1)[0]
    recorded_cpus = [int(cpu) for cpu in cpu_prefix.split(",") if cpu.strip()]
    check("affinity_cpus", recorded_cpus, slot_physical_cpus(resolved),
          note="fastchess names one logical CPU; Colosseum allocates the physical core")
    rows.append({"field": "engine_processes", "fastchess": "per-game",
                 "colosseum": resolved.get("engine_processes"), "equal": True,
                 "note": "differs by design: Colosseum keeps processes per slot"})
    return rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True, type=pathlib.Path)
    parser.add_argument("--dry-run", required=True, type=pathlib.Path, dest="dry_run")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()
    dry = json.loads(args.dry_run.read_text(encoding="utf-8-sig"))
    if dry.get("type") != "dry-run":
        raise SystemExit(f"{args.dry_run} is not a Colosseum dry run")
    rows = compare(read_manifest(args.manifest), dry)
    mismatches = [row for row in rows if not row["equal"]]
    if args.json:
        print(json.dumps({"rows": rows, "mismatches": len(mismatches)}, indent=2, default=str))
    else:
        for row in rows:
            print(f"{'ok' if row['equal'] else 'DIFF':4} {row['field']}: {row['fastchess']!r} / {row['colosseum']!r}")
        print(f"{len(rows) - len(mismatches)}/{len(rows)} fields agree", file=sys.stderr)
    return 1 if mismatches else 0


if __name__ == "__main__":
    raise SystemExit(main())
