#!/usr/bin/env python3
"""Validate and summarize the confound-corrected PLAN 6.1.c refinement."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

try:
    from .kbnk_sweep_summary import summarize_reports
except ImportError:  # Direct script execution puts this directory on sys.path.
    from kbnk_sweep_summary import summarize_reports

VARIANTS = {
    "legacy-baseline": "15600,800,900,220,220",
    "pass1-winner": "17000,1000,0,220,0",
    "d0900-k220": "15600,900,0,220,0",
    "d1000-k140": "15600,1000,0,140,0",
    "d1000-k220": "15600,1000,0,220,0",
    "d1000-k300": "15600,1000,0,300,0",
    "d1250-k140": "15600,1250,0,140,0",
    "d1250-k220": "15600,1250,0,220,0",
    "d1250-k300": "15600,1250,0,300,0",
    "d1450-k140": "15600,1450,0,140,0",
    "d1450-k220": "15600,1450,0,220,0",
    "d1450-k300": "15600,1450,0,300,0",
    "base14200": "14200,1250,0,220,0",
    "base17000": "17000,1250,0,220,0",
    "base18400": "18400,1250,0,220,0",
}
CONTROL_FINGERPRINTS = {
    # Canonical SHA-256 of the complete 60 per-position records returned by
    # the original four-field sweep. The corrected five-field controls must
    # reproduce these exactly or the refactor is not behavior-neutral.
    "legacy-baseline": "A28D2843263A8DDCB760C1133D5F105AE08E1F110AE4BB410210C683A15F1BF8",
    "pass1-winner": "792065BAC0D447E0B1577406103566AB0BDD65344BECF691747B20B2D085B5EB",
}


def validate_control_fingerprints(result_dir: Path) -> None:
    for name, expected in CONTROL_FINGERPRINTS.items():
        report = json.loads((result_dir / f"{name}.json").read_text(encoding="utf-8"))
        positions = report["families"]["KBN-K"]["positions"]
        canonical = json.dumps(positions, sort_keys=True, separators=(",", ":")).encode()
        actual = hashlib.sha256(canonical).hexdigest().upper()
        if actual != expected:
            raise ValueError(
                f"{name}: behavior-neutral control drift {actual} != {expected}"
            )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("result_dir", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    try:
        validate_control_fingerprints(args.result_dir)
        summarize_reports(
            args.result_dir,
            args.output or args.result_dir / "summary.json",
            VARIANTS,
            "legacy-baseline",
            "basilisk-kbnk-coefficient-refinement-v1",
            "6.1.c confound-corrected paired 60-position base/diagonal/king refinement",
        )
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
        parser.error(str(exc))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
