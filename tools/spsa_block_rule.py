"""Apply PLAN rule 7c's movement rule to a completed Colosseum block."""

from __future__ import annotations

import argparse
import json
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parent


def evaluate(result: dict, config: dict, minimum: int) -> tuple[str, list[tuple[str, int, int, float]]]:
    driver = result.get("driver", {})
    tuned = result.get("tuned_result", {})
    horizon = int(tuned.get("settings", {}).get("iterations", 0))
    done = int(tuned.get("completed_iterations", 0))
    if driver.get("status") != "completed" or not horizon or done != horizon:
        raise ValueError(f"not a completed block: status {driver.get('status')!r}, {done} of {horizon} iterations")
    parameters = tuned.get("parameters", [])
    names = [parameter["name"] for parameter in parameters]
    if names != list(config):
        raise ValueError(f"surface mismatch: run has {len(names)} coordinates, config has {len(config)}")
    movers = []
    for parameter in parameters:
        name = parameter["name"]
        original = int(parameter["original"])
        tuned_value = int(parameter["tuned"])
        moved = (tuned_value - original) / float(config[name]["step"])
        if abs(moved) >= 1.0:
            movers.append((name, original, tuned_value, moved))
    movers.sort(key=lambda item: -abs(item[3]))
    return ("CONTINUE" if len(movers) >= minimum else "STOP"), movers


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run_dir")
    parser.add_argument("--config", default=str(ROOT / "spsa_configs" / "config_search.json"))
    parser.add_argument("--min-moved", type=int, default=3)
    args = parser.parse_args()
    run = pathlib.Path(args.run_dir)
    result_path = run / "result.json"
    config_path = pathlib.Path(args.config)
    for path in (result_path, config_path):
        if not path.is_file():
            print(f"missing: {path}", file=sys.stderr)
            return 2
    try:
        result = json.loads(result_path.read_text(encoding="utf-8"))
        config = json.loads(config_path.read_text(encoding="utf-8"))
        verdict, movers = evaluate(result, config, args.min_moved)
    except (ValueError, KeyError, TypeError, json.JSONDecodeError) as error:
        print(error, file=sys.stderr)
        return 2
    count = len(result["tuned_result"]["parameters"])
    lines = [
        f"block {run}: {len(movers)} of {count} coordinates moved >= 1 step; "
        f"rule needs {args.min_moved}: {verdict}"
    ]
    lines.extend(f"  {name:32} {old:>8} -> {new:<8} {steps:+.2f} steps" for name, old, new, steps in movers)
    text = "\n".join(lines)
    print(text)
    (run / "block-rule.txt").write_text(text + "\n", encoding="utf-8")
    return 0 if verdict == "CONTINUE" else 1


if __name__ == "__main__":
    raise SystemExit(main())
