#!/usr/bin/env python3
"""Fail when PLAN.md and GUIDE.md checklist identifiers or states drift."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ITEM = re.compile(
    r"^\s*- \[(?P<state>[ x])\] \*\*(?P<id>\d+(?:\.\d+)*(?:\.[a-z])?)\*\*\s+"
)


def checklist(path: Path) -> dict[str, bool]:
    items: dict[str, bool] = {}
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        match = ITEM.match(line)
        if not match:
            continue
        key = match.group("id")
        if key in items:
            raise ValueError(f"{path}:{number}: duplicate checklist id {key}")
        items[key] = match.group("state") == "x"
    return items


def sort_key(identifier: str) -> tuple[tuple[int, int], ...]:
    parts = []
    for part in identifier.split("."):
        parts.append((0, int(part)) if part.isdigit() else (1, ord(part)))
    return tuple(parts)


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    plan = checklist(root / "PLAN.md")
    guide = checklist(root / "GUIDE.md")

    if plan != guide:
        missing = sorted(set(plan) - set(guide), key=sort_key)
        extra = sorted(set(guide) - set(plan), key=sort_key)
        changed = sorted(
            (key for key in set(plan) & set(guide) if plan[key] != guide[key]),
            key=sort_key,
        )
        if missing:
            print("GUIDE missing:", ", ".join(missing), file=sys.stderr)
        if extra:
            print("GUIDE extra:", ", ".join(extra), file=sys.stderr)
        if changed:
            print("state mismatch:", ", ".join(changed), file=sys.stderr)
        return 1

    complete = [key for key, done in plan.items() if done]
    open_items = [key for key, done in plan.items() if not done]
    if complete and open_items:
        last_done = max(complete, key=sort_key)
        first_open = min(open_items, key=sort_key)
        if sort_key(last_done) > sort_key(first_open):
            print(
                f"completed {last_done} sorts after open {first_open}",
                file=sys.stderr,
            )
            return 1

    print(
        f"roadmap synchronized: {len(complete)} complete, "
        f"{len(open_items)} open; next {min(open_items, key=sort_key)}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
