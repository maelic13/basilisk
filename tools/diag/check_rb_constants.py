#!/usr/bin/env python3
"""Verify that tools/diag/rb_structure_constants.tsv matches src/eval_rb*.h and src/eval.h."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_INVENTORY = ROOT / "tools" / "diag" / "rb_structure_constants.tsv"
SCHEMA = "# basilisk-rb-structure-constants-v1"
HEADER = ("name", "category", "file", "anchor", "value", "description")
INTERNAL_CONSTEXPR_ALLOWLIST = {"ALL_PIECES", "CONVERSION_S"}


def load_inventory(path: Path) -> list[dict[str, str]]:
    lines = path.read_text(encoding="utf-8").splitlines()
    if not lines or lines[0] != SCHEMA:
        raise ValueError(f"{path}: first line must be {SCHEMA!r}")
    data = [l for l in lines[1:] if l.strip() and not l.startswith("#")]
    if not data:
        raise ValueError(f"{path}: missing TSV header row")
    cols = tuple(data[0].split("\t"))
    if cols != HEADER:
        raise ValueError(f"{path}: expected columns {HEADER}, got {cols}")
    rows: list[dict[str, str]] = []
    seen_names: set[str] = set()
    for lineno, raw in enumerate(data[1:], start=1):
        parts = raw.split("\t")
        if len(parts) != len(HEADER):
            raise ValueError(f"{path}: row {lineno} has {len(parts)} columns, expected {len(HEADER)}")
        row = dict(zip(HEADER, parts))
        if any(not row[k].strip() for k in HEADER):
            raise ValueError(f"{path}: row {lineno} has an empty column: {row}")
        if row["name"] in seen_names:
            raise ValueError(f"{path}: duplicate entry {row['name']!r}")
        seen_names.add(row["name"])
        rows.append(row)
    return rows


def check_inventory(inventory_path: Path, root: Path = ROOT) -> list[str]:
    errors: list[str] = []
    try:
        rows = load_inventory(inventory_path)
    except (OSError, ValueError) as exc:
        return [str(exc)]

    files_cache: dict[str, str] = {}
    for row in rows:
        rel = row["file"]
        target = root / rel
        if not target.is_file():
            errors.append(f"{row['name']}: missing source file {rel}")
            continue
        text = files_cache.setdefault(rel, target.read_text(encoding="utf-8"))
        count = text.count(row["anchor"])
        if count != 1:
            errors.append(f"{row['name']}: anchor in {rel} matched {count} times (expected 1): {row['anchor']!r}")
            continue
        tokens = re.findall(r"\b\d+\b", row["anchor"])
        expected_vals = [v.strip() for v in row["value"].split(",")]
        for val in expected_vals:
            if val not in tokens:
                errors.append(f"{row['name']}: expected value {val!r} not found in anchor {row['anchor']!r}")

    # Completeness scan: every named integer structure constexpr in src/eval_rb*.h must be listed.
    listed_names = {row["name"] for row in rows}
    decl_pat = re.compile(r"^\s*(?:static\s+|inline\s+)?constexpr\s+int\s+([A-Z][A-Z0-9_]*)\s*=\s*-?\d+\s*;", re.M)
    for rb_file in sorted((root / "src").glob("eval_rb*.h")):
        rel = str(rb_file.relative_to(root)).replace("\\", "/")
        text = files_cache.setdefault(rel, rb_file.read_text(encoding="utf-8"))
        for match in decl_pat.finditer(text):
            cname = match.group(1)
            if cname not in INTERNAL_CONSTEXPR_ALLOWLIST and cname not in listed_names:
                errors.append(f"uninventoried constexpr int {cname!r} declared in {rel}")

    return errors


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--inventory", type=Path, default=DEFAULT_INVENTORY)
    ap.add_argument("--root", type=Path, default=ROOT)
    args = ap.parse_args(argv)
    errors = check_inventory(args.inventory, args.root)
    if errors:
        for err in errors:
            print(f"ERROR: {err}", file=sys.stderr)
        return 1
    rows = load_inventory(args.inventory)
    print(f"OK: {len(rows)} rebuilt-evaluation structure constants verified against source.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
