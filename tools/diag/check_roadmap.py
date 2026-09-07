#!/usr/bin/env python3
"""Fail on roadmap synchronization and obvious documentation-process drift."""

from __future__ import annotations

import re
import sys
import tempfile
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
CAPABILITY = re.compile(r"`\[(R3|R2|I2|I1|M|V)\]`")
LEGACY_CAPABILITY = re.compile(
    r"`\[(?:Astra|Fable|Sol|Terra|Sonnet)/(?:M|H|XH)\]`"
)
STATE_FIELD = re.compile(
    r"(?:\*\*)?State(?:\s*/\s*class)?\s*:(?:\*\*)?\s*`?([A-Z_]+)"
)
VALID_STATES = {
    "RESEARCH",
    "READY_FOR_IMPLEMENTATION",
    "IMPLEMENTED",
    "LOCAL_QUALIFIED",
    "GAME_GATE",
    "CLOSED",
}
EXPERIMENT_DEFINITION = re.compile(
    r"^(?:###\s+|\*\*|\|\s*)(BAS-[A-Z]\d+)(?:\s|\||\*)"
)
NEW_EXPERIMENT = re.compile(r"^###\s+(BAS-[A-Z]\d+)\b")
MARKDOWN_LINK = re.compile(r"\[[^\]]+\]\(([^)]+)\)")


def checklist(path: Path) -> tuple[dict[str, bool], set[str], dict[str, str]]:
    items: dict[str, bool] = {}
    reopened: set[str] = set()
    capabilities: dict[str, str] = {}
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
        tags = CAPABILITY.findall(match.group("rest"))
        if len(tags) > 1:
            raise ValueError(f"{path}:{number}: multiple capability tags on {key}")
        if tags:
            capabilities[key] = tags[0]
        if not items[key] and LEGACY_CAPABILITY.search(match.group("rest")):
            raise ValueError(f"{path}:{number}: legacy model tag on open item {key}")

    parents = {
        key for key in items if any(other.startswith(key + ".") for other in items)
    }
    for key, done in items.items():
        if not done and key not in parents and key not in capabilities:
            raise ValueError(f"{path}: open leaf {key} lacks a valid capability tag")
    return items, reopened, capabilities


def validate_state_fields(paths: list[Path]) -> None:
    for path in paths:
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            match = STATE_FIELD.search(line)
            if match and match.group(1) not in VALID_STATES:
                raise ValueError(
                    f"{path}:{number}: invalid workflow state {match.group(1)}"
                )


def validate_capability_match(
    plan_capabilities: dict[str, str], guide_capabilities: dict[str, str]
) -> None:
    if plan_capabilities == guide_capabilities:
        return
    changed = sorted(
        key for key in set(plan_capabilities) | set(guide_capabilities)
        if plan_capabilities.get(key) != guide_capabilities.get(key)
    )
    raise ValueError("capability mismatch: " + ", ".join(changed))


def validate_experiment_ids(path: Path) -> None:
    definitions: dict[str, list[int]] = {}
    new_ids: set[str] = set()
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        match = EXPERIMENT_DEFINITION.match(line)
        if match:
            definitions.setdefault(match.group(1), []).append(number)
        new_match = NEW_EXPERIMENT.match(line)
        if new_match:
            key = new_match.group(1)
            if key in new_ids:
                raise ValueError(f"{path}:{number}: duplicate new experiment id {key}")
            new_ids.add(key)
    for key in new_ids:
        if len(definitions.get(key, [])) != 1:
            raise ValueError(
                f"{path}: new experiment id {key} collides at lines "
                f"{definitions.get(key, [])}"
            )


def validate_local_links(paths: list[Path]) -> None:
    for path in paths:
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            for raw_target in MARKDOWN_LINK.findall(line):
                target = raw_target.strip().strip("<>").split("#", 1)[0]
                if not target or "://" in target or target.startswith(("mailto:", "#")):
                    continue
                if not (path.parent / target).resolve().exists():
                    raise ValueError(f"{path}:{number}: broken local link {raw_target}")


def self_test() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        valid = "- [ ] **1.0** Parent\n  - [ ] **1.0.a** `[R2]` Leaf\n"
        (root / "PLAN.md").write_text(valid, encoding="utf-8")
        checklist(root / "PLAN.md")

        (root / "PLAN.md").write_text(valid.replace("[R2]", "[R4]"), encoding="utf-8")
        try:
            checklist(root / "PLAN.md")
        except ValueError:
            pass
        else:
            raise AssertionError("malformed capability tag was accepted")

        try:
            validate_capability_match({"1.0.a": "R2"}, {"1.0.a": "I1"})
        except ValueError:
            pass
        else:
            raise AssertionError("PLAN/GUIDE capability mismatch was accepted")

        (root / "state.md").write_text("**State:** INVENTED\n", encoding="utf-8")
        try:
            validate_state_fields([root / "state.md"])
        except ValueError:
            pass
        else:
            raise AssertionError("invalid workflow state was accepted")

        (root / "EXPERIMENTS.md").write_text(
            "### BAS-E99 — first\n### BAS-E99 — duplicate\n", encoding="utf-8"
        )
        try:
            validate_experiment_ids(root / "EXPERIMENTS.md")
        except ValueError:
            pass
        else:
            raise AssertionError("duplicate experiment id was accepted")

        (root / "doc.md").write_text("[missing](nope.md)\n", encoding="utf-8")
        try:
            validate_local_links([root / "doc.md"])
        except ValueError:
            pass
        else:
            raise AssertionError("broken local link was accepted")
    print("roadmap checker self-test: known-bad inputs rejected")


def sort_key(identifier: str) -> tuple[tuple[int, int], ...]:
    parts = []
    for part in identifier.split("."):
        parts.append((0, int(part)) if part.isdigit() else (1, ord(part)))
    return tuple(parts)


def main() -> int:
    if "--self-test" in sys.argv[1:]:
        self_test()
        return 0

    root = Path(__file__).resolve().parents[2]
    plan, plan_reopened, plan_capabilities = checklist(root / "PLAN.md")
    guide, guide_reopened, guide_capabilities = checklist(root / "GUIDE.md")

    validate_capability_match(plan_capabilities, guide_capabilities)

    process_docs = [
        root / "AGENTS.md",
        root / "GUIDE.md",
        root / "PLAN.md",
        root / "EXPERIMENTS.md",
        root / "DESIGN.md",
        root / "analysis" / "README.md",
    ]
    validate_state_fields(process_docs)
    validate_experiment_ids(root / "EXPERIMENTS.md")
    validate_local_links(process_docs)

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
