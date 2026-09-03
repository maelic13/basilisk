#!/usr/bin/env python3
"""Fail when PLAN.md and GUIDE.md checklist identifiers or states drift."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ITEM = re.compile(
    r"^\s*- \[(?P<state>[ x])\] \*\*(?P<id>\d+(?:\.\d+)*(?:\.[a-z])?)\*\*\s+"
    r"(?P<rest>.*)$"
)
# A leaf that was completed and later invalidated is reopened deliberately, and
# then sits BEFORE work that is already finished. That is a legitimate state --
# a measurement defect can invalidate an early step after later ones closed --
# but it must be declared, so that an accidental un-tick still fails the order
# check. Marked items are exempt from ordering only, never from being open.
REOPENED = "(REOPENED)"


def checklist(path: Path) -> tuple[dict[str, bool], set[str]]:
    items: dict[str, bool] = {}
    reopened: set[str] = set()
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        match = ITEM.match(line)
        if not match:
            continue
        key = match.group("id")
        if key in items:
            raise ValueError(f"{path}:{number}: duplicate checklist id {key}")
        items[key] = match.group("state") == "x"
        if REOPENED in match.group("rest"):
            if items[key]:
                raise ValueError(
                    f"{path}:{number}: {key} is marked {REOPENED} but ticked"
                )
            reopened.add(key)
    return items, reopened


def sort_key(identifier: str) -> tuple[tuple[int, int], ...]:
    parts = []
    for part in identifier.split("."):
        parts.append((0, int(part)) if part.isdigit() else (1, ord(part)))
    return tuple(parts)


def main() -> int:
    root = Path(__file__).resolve().parents[2]
    plan, plan_reopened = checklist(root / "PLAN.md")
    guide, guide_reopened = checklist(root / "GUIDE.md")

    # Both files must agree on WHICH items are reopened, for the same reason
    # they must agree on which are ticked.
    if plan_reopened != guide_reopened:
        only_plan = sorted(plan_reopened - guide_reopened, key=sort_key)
        only_guide = sorted(guide_reopened - plan_reopened, key=sort_key)
        print(
            f"reopened markers differ: PLAN-only {only_plan}, GUIDE-only {only_guide}",
            file=sys.stderr,
        )
        return 1
    reopened = plan_reopened

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

    parents = {
        key for key in plan
        if any(other.startswith(key + ".") for other in plan)
    }
    for parent in sorted(parents, key=sort_key):
        descendants = [
            done for key, done in plan.items() if key.startswith(parent + ".")
        ]
        expected = all(descendants)
        if plan[parent] != expected:
            state = "complete" if expected else "open"
            print(
                f"parent {parent} must be {state} to match its substeps",
                file=sys.stderr,
            )
            return 1

    leaves = {key: done for key, done in plan.items() if key not in parents}
    complete_leaves = [key for key, done in leaves.items() if done]
    open_leaves = [key for key, done in leaves.items() if not done]
    ordered_open = [key for key in open_leaves if key not in reopened]
    if complete_leaves and ordered_open:
        last_done = max(complete_leaves, key=sort_key)
        first_open = min(ordered_open, key=sort_key)
        if sort_key(last_done) > sort_key(first_open):
            print(
                f"completed {last_done} sorts after open {first_open}",
                file=sys.stderr,
            )
            return 1

    if reopened:
        print(
            "REOPENED and needing rework: "
            + ", ".join(sorted(reopened, key=sort_key))
        )
    print(
        f"roadmap synchronized: {sum(plan.values())} complete, "
        f"{len(plan) - sum(plan.values())} open; "
        f"next {min(open_leaves, key=sort_key) if open_leaves else 'none'}"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
