#!/usr/bin/env python3
"""Validate and summarize the PLAN 6.1.c upper-range KBNK sweep."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

try:
    from .kbnk_sweep_summary import summarize_reports
except ImportError:
    from kbnk_sweep_summary import summarize_reports

VARIANTS = {
    "legacy-baseline": "15600,800,900,220,220",
    "boundary-control": "15600,1450,0,300,0",
    "d1350-k340": "15600,1350,0,340,0",
    "d1350-k460": "15600,1350,0,460,0",
    "d1450-k340": "15600,1450,0,340,0",
    "d1450-k460": "15600,1450,0,460,0",
    "d1550-k340": "15600,1550,0,340,0",
    "d1550-k460": "15600,1550,0,460,0",
    "d1650-k460": "15600,1650,0,460,0",
    "d1750-k340": "15600,1750,0,340,0",
    "d1750-k460": "15600,1750,0,460,0",
    "d1850-k340": "15600,1850,0,340,0",
    "d1850-k460": "15600,1850,0,460,0",
    "d1900-k340": "15600,1900,0,340,0",
    "d1900-k460": "15600,1900,0,460,0",
}
CONTROL_FINGERPRINTS = {
    "legacy-baseline": "A28D2843263A8DDCB760C1133D5F105AE08E1F110AE4BB410210C683A15F1BF8",
    "boundary-control": "ACB112FE78F1C074CD9C49AD16641BF8B1DA9DB872A9386BFC6DF672C964869E",
}


def validate_control_fingerprints(result_dir: Path) -> None:
    for name, expected in CONTROL_FINGERPRINTS.items():
        report = json.loads((result_dir / f"{name}.json").read_text(encoding="utf-8"))
        positions = report["families"]["KBN-K"]["positions"]
        canonical = json.dumps(positions, sort_keys=True, separators=(",", ":")).encode()
        actual = hashlib.sha256(canonical).hexdigest().upper()
        if actual != expected:
            raise ValueError(f"{name}: control drift {actual} != {expected}")


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
            "basilisk-kbnk-upper-sweep-v1",
            "6.1.c paired upper-range screen bounded by live-truth safety",
        )
    except (OSError, ValueError, KeyError, json.JSONDecodeError) as exc:
        parser.error(str(exc))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
