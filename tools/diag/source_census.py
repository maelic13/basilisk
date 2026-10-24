#!/usr/bin/env python3
"""Mechanical census of the engine source for an architecture review.

For every file under src/ it reports lines, comment lines, the comment share
and the number of references to retired roadmap numbering (phase and step
numbers, lettered leaves, ledger IDs) inside comments; the include edges
between the search modules; and the worker's coupling to its state by
region. The output is deterministic so two reviews of different heads can be
compared line for line. No figure here is evidence about play.

Usage:
  python tools/diag/source_census.py [--root DIR] [--json OUT.json]

The retired-numbering pattern deliberately over-matches (a version number or
a decimal constant in a comment counts), so the figure is an upper bound and
only its change between heads is meaningful.
"""

from __future__ import annotations

import argparse
import json
import pathlib
import re
import sys

RETIRED = re.compile(
    r"(\bPhase\s*\d|\bStep\s*\d|\b(?:BAS|RAR)-[A-Z]\d+|\b[A-G]\.\d+(?:\.\d+)*\b"
    r"|\b(?:[1-9]|1[0-5])\.\d{1,2}(?:\.\d{1,2})?(?:\s*\([a-z]\)|[a-z]\b|\b))"
)
INCLUDE = re.compile(r'#include "([a-z_0-9]+\.h)"')
SEARCH_MODULES = (
    "search.h", "search_types.h", "search_root.h", "search_root.cpp",
    "search_diagnostics.h", "search_diagnostics.cpp", "search_params.h",
    "search_worker.h", "search_worker.cpp", "search_kernel.cpp",
    "search_history.cpp", "search_thread_pool.h", "search_thread_pool.cpp",
    "move_picker.h", "move_picker.cpp", "history.h", "history.cpp", "tt.h",
)
# Member-state accesses the worker's regions are measured by.
ACCESS = {
    "params": re.compile(r"config_\.limits\.params"),
    "state": re.compile(r"\bstate_\."),
    "hist": re.compile(r"\bhist_\."),
    "shared": re.compile(r"\bshared_\."),
    "eval": re.compile(r"\bevaluator_\."),
}


def comment_census(path: pathlib.Path) -> dict:
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    comment = 0
    retired = 0
    in_block = False
    for line in lines:
        stripped = line.strip()
        if in_block:
            comment += 1
            retired += len(RETIRED.findall(stripped))
            if "*/" in stripped:
                in_block = False
            continue
        if stripped.startswith("//"):
            comment += 1
            retired += len(RETIRED.findall(stripped))
        elif stripped.startswith("/*"):
            comment += 1
            retired += len(RETIRED.findall(stripped))
            in_block = "*/" not in stripped
        elif "//" in line:
            retired += len(RETIRED.findall(line[line.index("//"):]))
    return {
        "file": path.name,
        "lines": len(lines),
        "comment_lines": comment,
        "comment_share": round(100.0 * comment / max(len(lines), 1), 1),
        "retired_refs": retired,
    }


def include_edges(src: pathlib.Path) -> dict[str, list[str]]:
    edges: dict[str, list[str]] = {}
    for name in SEARCH_MODULES:
        path = src / name
        if not path.exists():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        edges[name] = INCLUDE.findall(text)
    return edges


def access_census(src: pathlib.Path) -> dict[str, dict[str, int]]:
    out: dict[str, dict[str, int]] = {}
    for name in ("search_worker.cpp", "search_kernel.cpp", "search_history.cpp"):
        path = src / name
        if not path.exists():
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        out[name] = {key: len(rx.findall(text)) for key, rx in ACCESS.items()}
        out[name]["lines"] = text.count("\n") + 1
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--root", default=str(pathlib.Path(__file__).resolve().parents[2]))
    parser.add_argument("--json", default="")
    args = parser.parse_args()
    src = pathlib.Path(args.root) / "src"
    files = sorted(list(src.glob("*.h")) + list(src.glob("*.cpp")), key=lambda p: p.name)
    rows = [comment_census(p) for p in files]
    rows.sort(key=lambda r: (-r["retired_refs"], r["file"]))
    total_lines = sum(r["lines"] for r in rows)
    total_comment = sum(r["comment_lines"] for r in rows)
    total_retired = sum(r["retired_refs"] for r in rows)

    print(f"{'file':28} {'lines':>6} {'comment':>8} {'share':>6} {'retired':>8}")
    for r in rows:
        print(f"{r['file']:28} {r['lines']:6} {r['comment_lines']:8} {r['comment_share']:5.0f}% {r['retired_refs']:8}")
    print(f"{'TOTAL':28} {total_lines:6} {total_comment:8} {100.0 * total_comment / total_lines:5.0f}% {total_retired:8}")

    edges = include_edges(src)
    print("\ninclude edges (search modules)")
    for name, deps in edges.items():
        print(f"  {name}: {' '.join(deps)}")

    access = access_census(src)
    print("\nworker state accesses by file")
    for name, counts in access.items():
        print("  " + name + ": " + " ".join(f"{k}={v}" for k, v in counts.items()))

    if args.json:
        payload = {
            "files": rows,
            "totals": {"lines": total_lines, "comment_lines": total_comment, "retired_refs": total_retired},
            "include_edges": edges,
            "worker_access": access,
        }
        pathlib.Path(args.json).write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
