#!/usr/bin/env python3
"""Fail on roadmap synchronization and obvious documentation-process drift."""

from __future__ import annotations

import hashlib
import re
import sys
import tempfile
from pathlib import Path

# Leaf identifiers are either lettered (`A.1`, `B.2.0.1`, the current roadmap)
# or numeric (`6.6.a`, the retired roadmaps kept under docs/archive/).
ITEM = re.compile(
    r"^\s*- \[(?P<state>[ x])\] \*\*(?P<id>(?:[A-Z]|\d+)(?:\.\d+)*(?:\.[a-z])?)\*\*\s+"
    r"(?P<rest>.*)$"
)
# A leaf that was completed and later invalidated is reopened deliberately, and
# then sits BEFORE work that is already finished. That is a legitimate state --
# a measurement defect can invalidate an early step after later ones closed --
# but it must be declared, so that an accidental un-tick still fails the order
# check. Marked items are exempt from ordering only, never from being open.
REOPENED = "(REOPENED)"
# A leaf that depends on nothing before it may land at any point before its
# stated deadline (a tooling flow, a guard test). It must be declared for the
# same reason as a reopened leaf: ordering is the default, and an undeclared
# out-of-order tick is still an error.
ANY_TIME = "(ANY TIME)"
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
REGISTER_HEADING = "active workflow register"
REGISTER_ROW = re.compile(
    r"^\|\s*(?P<id>[A-Z](?:\.\d+)+)\s*\|\s*`?(?P<state>[A-Z_]+)`?\s*\|"
    r"\s*`?(?P<cls>R3|R2|I2|I1|M|V)`?\s*\|"
)
FINGERPRINT = re.compile(r"\*\*(\d{1,3}(?:,\d{3})+)\*\*")
GUIDE_FINGERPRINT_ROW = "| Bench fingerprint |"
# Each document that restates the current fingerprint does it on one line
# containing this anchor, so a stale copy cannot hide behind a line wrap.
FINGERPRINT_ANCHORS = (("DESIGN.md", "Bench signature"), ("AGENTS.md", "currently **"))


class Checklist:
    def __init__(self) -> None:
        self.items: dict[str, bool] = {}
        self.reopened: set[str] = set()
        self.any_time: set[str] = set()
        self.capabilities: dict[str, str] = {}

    @property
    def parents(self) -> set[str]:
        return {
            key for key in self.items
            if any(other.startswith(key + ".") for other in self.items)
        }


def checklist(path: Path) -> Checklist:
    result = Checklist()
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        match = ITEM.match(line)
        if not match:
            continue
        key = match.group("id")
        if key in result.items:
            raise ValueError(f"{path}:{number}: duplicate checklist id {key}")
        done = match.group("state") == "x"
        result.items[key] = done
        rest = match.group("rest")
        if REOPENED in rest:
            if done:
                raise ValueError(
                    f"{path}:{number}: {key} is marked {REOPENED} but ticked"
                )
            result.reopened.add(key)
        if ANY_TIME in rest:
            result.any_time.add(key)
        tags = CAPABILITY.findall(rest)
        if len(tags) > 1:
            raise ValueError(f"{path}:{number}: multiple capability tags on {key}")
        if tags:
            result.capabilities[key] = tags[0]
        if not done and LEGACY_CAPABILITY.search(rest):
            raise ValueError(f"{path}:{number}: legacy model tag on open item {key}")

    parents = result.parents
    for key, done in result.items.items():
        if not done and key not in parents and key not in result.capabilities:
            raise ValueError(f"{path}: open leaf {key} lacks a valid capability tag")
    return result


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


def sort_key(identifier: str) -> tuple[tuple[int, int], ...]:
    parts = []
    for part in identifier.split("."):
        parts.append((0, int(part)) if part.isdigit() else (1, ord(part)))
    return tuple(parts)


def validate_parents(plan: Checklist) -> None:
    for parent in sorted(plan.parents, key=sort_key):
        descendants = [
            done for key, done in plan.items.items() if key.startswith(parent + ".")
        ]
        expected = all(descendants)
        if plan.items[parent] != expected:
            state = "complete" if expected else "open"
            raise ValueError(f"parent {parent} must be {state} to match its substeps")


def ordered_open_leaves(plan: Checklist) -> list[str]:
    parents = plan.parents
    return sorted(
        (
            key for key, done in plan.items.items()
            if not done and key not in parents
            and key not in plan.reopened and key not in plan.any_time
        ),
        key=sort_key,
    )


def validate_order(plan: Checklist) -> None:
    parents = plan.parents
    complete = [
        key for key, done in plan.items.items()
        if done and key not in parents and key not in plan.any_time
    ]
    ordered_open = ordered_open_leaves(plan)
    if complete and ordered_open:
        last_done = max(complete, key=sort_key)
        first_open = ordered_open[0]
        if sort_key(last_done) > sort_key(first_open):
            raise ValueError(f"completed {last_done} sorts after open {first_open}")


def register_rows(path: Path) -> dict[str, tuple[str, str, int]]:
    rows: dict[str, tuple[str, str, int]] = {}
    in_register = False
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if line.startswith("#"):
            in_register = REGISTER_HEADING in line.lower()
            continue
        if not in_register:
            continue
        match = REGISTER_ROW.match(line)
        if not match:
            continue
        key = match.group("id")
        if key in rows:
            raise ValueError(f"{path}:{number}: duplicate register row {key}")
        rows[key] = (match.group("state"), match.group("cls"), number)
    return rows


def validate_register(plan: Checklist, rows: dict[str, tuple[str, str, int]]) -> None:
    """Register rows name open leaves; the current phase's open leaves all have one."""
    parents = plan.parents
    for key, (state, cls, number) in rows.items():
        where = f"PLAN.md:{number}"
        if state not in VALID_STATES:
            raise ValueError(f"{where}: invalid workflow state {state} for {key}")
        if key not in plan.items:
            raise ValueError(f"{where}: register row {key} names no checklist item")
        if plan.items[key]:
            raise ValueError(f"{where}: register row {key} is for a completed item")
        if key in parents:
            raise ValueError(f"{where}: register row {key} names a parent, not a leaf")
        if plan.capabilities.get(key) != cls:
            raise ValueError(
                f"{where}: register class {cls} for {key} disagrees with its tag "
                f"{plan.capabilities.get(key)}"
            )
    ordered_open = ordered_open_leaves(plan)
    if not ordered_open:
        return
    phase = ordered_open[0].split(".")[0]
    missing = sorted(
        (
            key for key, done in plan.items.items()
            if not done and key not in parents and key.split(".")[0] == phase
            and key not in rows
        ),
        key=sort_key,
    )
    if missing:
        raise ValueError(
            f"open leaves of current phase {phase} lack register rows: "
            + ", ".join(missing)
        )


def validate_fingerprint(root: Path) -> str:
    declared = None
    for line in (root / "GUIDE.md").read_text(encoding="utf-8").splitlines():
        if line.startswith(GUIDE_FINGERPRINT_ROW):
            match = FINGERPRINT.search(line)
            if match:
                declared = match.group(1)
            break
    if declared is None:
        raise ValueError("GUIDE.md checkpoint declares no bold bench fingerprint")
    for name, anchor in FINGERPRINT_ANCHORS:
        lines = [
            line for line in (root / name).read_text(encoding="utf-8").splitlines()
            if anchor in line
        ]
        if not lines:
            raise ValueError(f"{name}: no line contains the fingerprint anchor {anchor!r}")
        for line in lines:
            if declared not in FINGERPRINT.findall(line):
                raise ValueError(
                    f"{name}: the {anchor!r} line does not state GUIDE's fingerprint "
                    f"{declared}"
                )
    return declared


def validate_reference_manifests(root: Path) -> int:
    """Each docs/reference/<name>.sha256 lists exactly the files of <name>/, unchanged."""
    reference = root / "docs" / "reference"
    count = 0
    for manifest in sorted(reference.glob("*.sha256")):
        snapshot = manifest.with_suffix("")
        listed: dict[str, str] = {}
        for number, line in enumerate(manifest.read_text(encoding="utf-8").splitlines(), 1):
            if not line.strip():
                continue
            digest, _, rel = line.partition("  ")
            if len(digest) != 64 or not rel:
                raise ValueError(f"{manifest}:{number}: malformed manifest line")
            listed[rel] = digest
        present = {
            path.relative_to(snapshot).as_posix()
            for path in snapshot.rglob("*") if path.is_file()
        }
        if present != set(listed):
            unlisted = sorted(present - set(listed))[:5]
            absent = sorted(set(listed) - present)[:5]
            raise ValueError(
                f"{snapshot}: files differ from {manifest.name}: "
                f"unlisted {unlisted}, absent {absent}"
            )
        for rel, digest in listed.items():
            if hashlib.sha256((snapshot / rel).read_bytes()).hexdigest() != digest:
                raise ValueError(f"{snapshot / rel}: changed since its snapshot manifest")
        count += len(listed)
    return count


def expect_failure(label: str, action) -> None:
    try:
        action()
    except ValueError:
        return
    raise AssertionError(f"{label} was accepted")


def self_test() -> None:
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        valid = "- [ ] **1.0** Parent\n  - [ ] **1.0.a** `[R2]` Leaf\n"
        (root / "PLAN.md").write_text(valid, encoding="utf-8")
        checklist(root / "PLAN.md")

        (root / "PLAN.md").write_text(valid.replace("[R2]", "[R4]"), encoding="utf-8")
        expect_failure("malformed capability tag", lambda: checklist(root / "PLAN.md"))

        expect_failure(
            "PLAN/GUIDE capability mismatch",
            lambda: validate_capability_match({"1.0.a": "R2"}, {"1.0.a": "I1"}),
        )

        (root / "state.md").write_text("**State:** INVENTED\n", encoding="utf-8")
        expect_failure("invalid workflow state", lambda: validate_state_fields([root / "state.md"]))

        (root / "EXPERIMENTS.md").write_text(
            "### BAS-E99 — first\n### BAS-E99 — duplicate\n", encoding="utf-8"
        )
        expect_failure("duplicate experiment id", lambda: validate_experiment_ids(root / "EXPERIMENTS.md"))

        (root / "doc.md").write_text("[missing](nope.md)\n", encoding="utf-8")
        expect_failure("broken local link", lambda: validate_local_links([root / "doc.md"]))

        # Lettered identifiers, the ordering rule and its two declared exemptions.
        lettered = (
            "- [x] **A.1** `[M]` Done\n"
            "- [ ] **A.2** Parent\n"
            "    - [ ] **A.2.1** `[I1]` Open\n"
            "- [ ] **E.3** Parent\n"
            "    - [x] **E.3.1** `[I1]` Early {marker}\n"
            "    - [ ] **E.3.2** `[M]` Late\n"
        )
        (root / "PLAN.md").write_text(lettered.format(marker=ANY_TIME), encoding="utf-8")
        plan = checklist(root / "PLAN.md")
        if set(plan.items) != {"A.1", "A.2", "A.2.1", "E.3", "E.3.1", "E.3.2"}:
            raise AssertionError(f"lettered identifiers misparsed: {sorted(plan.items)}")
        validate_order(plan)
        validate_parents(plan)
        (root / "PLAN.md").write_text(lettered.format(marker=""), encoding="utf-8")
        expect_failure(
            "an undeclared out-of-order completion",
            lambda: validate_order(checklist(root / "PLAN.md")),
        )

        # The register: rows must match the leaf's tag and cover the current phase.
        (root / "PLAN.md").write_text(
            lettered.format(marker=ANY_TIME)
            + "\n### Active workflow register\n\n| Leaf | State | Class |\n|---|---|---|\n"
            + "| A.2.1 | RESEARCH | I1 |\n",
            encoding="utf-8",
        )
        plan = checklist(root / "PLAN.md")
        validate_register(plan, register_rows(root / "PLAN.md"))
        expect_failure(
            "a register class that disagrees with the leaf's tag",
            lambda: validate_register(plan, {"A.2.1": ("RESEARCH", "R3", 1)}),
        )
        expect_failure(
            "a current-phase open leaf without a register row",
            lambda: validate_register(plan, {}),
        )
        expect_failure(
            "a register row for a completed leaf",
            lambda: validate_register(plan, {"A.2.1": ("RESEARCH", "I1", 1), "A.1": ("RESEARCH", "M", 2)}),
        )

        # One declared fingerprint, restated identically where it is restated.
        (root / "GUIDE.md").write_text("| Bench fingerprint | **1,234,567** |\n", encoding="utf-8")
        (root / "DESIGN.md").write_text("- **Bench signature**: **1,234,567**\n", encoding="utf-8")
        (root / "AGENTS.md").write_text("(currently **1,234,567**)\n", encoding="utf-8")
        validate_fingerprint(root)
        (root / "DESIGN.md").write_text("- **Bench signature**: **7,654,321**\n", encoding="utf-8")
        expect_failure("a stale restated fingerprint", lambda: validate_fingerprint(root))

        # A reference snapshot verifies against its manifest, and nothing else does.
        snapshot = root / "docs" / "reference" / "donor"
        snapshot.mkdir(parents=True)
        (snapshot / "a.md").write_bytes(b"verbatim\n")
        digest = hashlib.sha256(b"verbatim\n").hexdigest()
        (root / "docs" / "reference" / "donor.sha256").write_text(f"{digest}  a.md\n", encoding="utf-8")
        if validate_reference_manifests(root) != 1:
            raise AssertionError("reference manifest was not read")
        (snapshot / "a.md").write_bytes(b"edited\n")
        expect_failure("an edited reference snapshot", lambda: validate_reference_manifests(root))
        (snapshot / "a.md").write_bytes(b"verbatim\n")
        (snapshot / "extra.md").write_bytes(b"x\n")
        expect_failure("an unlisted file in a reference snapshot", lambda: validate_reference_manifests(root))
    print("roadmap checker self-test: known-bad inputs rejected")


def main() -> int:
    if "--self-test" in sys.argv[1:]:
        self_test()
        return 0

    root = Path(__file__).resolve().parents[2]
    try:
        plan = checklist(root / "PLAN.md")
        guide = checklist(root / "GUIDE.md")
        validate_capability_match(plan.capabilities, guide.capabilities)

        process_docs = [
            root / "AGENTS.md",
            root / "GUIDE.md",
            root / "PLAN.md",
            root / "PROCESS.md",
            root / "EXPERIMENTS.md",
            root / "DESIGN.md",
            root / "HISTORY.md",
            root / "analysis" / "README.md",
            root / "docs" / "reference" / "README.md",
        ]
        process_docs = [path for path in process_docs if path.exists()]
        validate_state_fields(process_docs)
        validate_experiment_ids(root / "EXPERIMENTS.md")
        validate_local_links(process_docs)

        # Both files must agree on WHICH items carry an ordering exemption, for
        # the same reason they must agree on which are ticked.
        for label, plan_set, guide_set in (
            (REOPENED, plan.reopened, guide.reopened),
            (ANY_TIME, plan.any_time, guide.any_time),
        ):
            if plan_set != guide_set:
                only_plan = sorted(plan_set - guide_set, key=sort_key)
                only_guide = sorted(guide_set - plan_set, key=sort_key)
                raise ValueError(
                    f"{label} markers differ: PLAN-only {only_plan}, GUIDE-only {only_guide}"
                )

        if plan.items != guide.items:
            missing = sorted(set(plan.items) - set(guide.items), key=sort_key)
            extra = sorted(set(guide.items) - set(plan.items), key=sort_key)
            changed = sorted(
                (key for key in set(plan.items) & set(guide.items)
                 if plan.items[key] != guide.items[key]),
                key=sort_key,
            )
            problems = []
            if missing:
                problems.append("GUIDE missing: " + ", ".join(missing))
            if extra:
                problems.append("GUIDE extra: " + ", ".join(extra))
            if changed:
                problems.append("state mismatch: " + ", ".join(changed))
            raise ValueError("; ".join(problems))

        validate_parents(plan)
        validate_order(plan)
        validate_register(plan, register_rows(root / "PLAN.md"))
        fingerprint = validate_fingerprint(root)
        reference_files = validate_reference_manifests(root)
    except ValueError as error:
        print(error, file=sys.stderr)
        return 1

    if plan.reopened:
        print(
            "REOPENED and needing rework: "
            + ", ".join(sorted(plan.reopened, key=sort_key))
        )
    ordered_open = ordered_open_leaves(plan)
    print(
        f"roadmap synchronized: {sum(plan.items.values())} complete, "
        f"{len(plan.items) - sum(plan.items.values())} open; "
        f"next {ordered_open[0] if ordered_open else 'none'}; "
        f"fingerprint {fingerprint}; {reference_files} reference files verified"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
