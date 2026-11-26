#!/usr/bin/env python3
"""Keep the agent rules within what every agent tool loads.

AGENTS.md is the core, which every agent loads; the files under agents/ are
modules, read in full when a trigger in the core's index is true. Agent tools
load a rule file only up to a size limit and drop the rest without a warning,
and a rulebook grows by one rule per incident. This check fails when:

- a set of files a tool loads by itself exceeds the budget the core declares:
  a stub and everything it imports, the core included;
- a module exceeds the same budget, so it can always be read in one go;
- a module is missing from the core's index, or the index names a file that
  does not exist;
- a module does not open with its scope and triggers, word for word as the
  index gives them;
- a stub imports a module, which would load it on every task;
- a rule file sits where a tool would load it and this check does not know it;
- the core has more lines than it declares, where it declares a number.

The index is a table in the core, one row for each module: its path, its
scope, and its triggers by kind (the task, the paths changed, the actions).

`--triggered` lists the modules whose path triggers the current branch's
changes fire, for the closing check in AGENTS.md.
"""

from __future__ import annotations

import argparse
import fnmatch
import re
import subprocess
import sys
from pathlib import Path

CORE = "AGENTS.md"
MODULE_DIR = "agents"

# The tools these rules are kept for (maintainer, 2026-11-25): Claude, Codex,
# Antigravity and Gemini CLI. No other tool's files or limits are considered.
# What each loads at the start of every session, by path or directory
# under the root. A tool that finds none of its own files falls back to the
# core, which is why the core alone is a set as well.
TOOL_FILES = {
    "Claude Code": ("CLAUDE.md", "CLAUDE.local.md", ".claude/CLAUDE.md", ".claude/rules/"),
    "Codex": ("AGENTS.override.md",),
    "Gemini CLI and Antigravity": ("GEMINI.md", ".agent/rules/", ".agents/rules/"),
}
# Names a tool loads from the directory an agent works in, wherever it is.
RULE_FILE_NAMES = {"AGENTS.md", "AGENTS.override.md", "CLAUDE.md", "CLAUDE.local.md", "GEMINI.md"}

BUDGET = re.compile(r"Budget: \*\*(\d{1,3}(?:,\d{3})*)\*\* bytes")
LINES = re.compile(r"and \*\*(\d+)\*\* lines for this file")
INDEX_ROW = re.compile(r"^\|\s*`(" + MODULE_DIR + r"/[\w.-]+\.md)`\s*\|(.*)\|\s*$")
INDEX_COLUMNS = ("Scope", "Task", "Paths", "Actions")
FENCE = re.compile(r"^(```|~~~).*?^\1[^\n]*$", re.MULTILINE | re.DOTALL)
CODE_SPAN = re.compile(r"`[^`\n]*`")
# Claude Code and Gemini CLI import `@path`; Antigravity inlines `@[label](path)`.
IMPORT = re.compile(r"(?<![\w@/.])@(?:\[[^\]\n]*\]\(([^)\s]+)\)|([\w~.][\w~./\\-]*))")
PATHS_CLAUSE = re.compile(r"Paths --(.*?)(?:Actions --|Task --|$)")


class CheckError(Exception):
    pass


def read(path: Path) -> str:
    return path.read_bytes().decode("utf-8").replace("\r\n", "\n")


def loaded_bytes(path: Path) -> int:
    """UTF-8 bytes as a CRLF checkout holds the file: the larger of its two forms."""
    text = read(path)
    return len(text.encode("utf-8")) + text.count("\n")


def squeeze(text: str) -> str:
    return " ".join(text.split())


def tracked_files(root: Path) -> list[str]:
    """Tracked and unignored files; every file when the root is not a git work tree."""
    try:
        listed = subprocess.run(
            ["git", "-C", str(root), "ls-files", "-z", "--cached", "--others", "--exclude-standard"],
            capture_output=True, check=True,
        ).stdout.decode("utf-8")
        inside = subprocess.run(
            ["git", "-C", str(root), "rev-parse", "--show-toplevel"],
            capture_output=True, check=True,
        ).stdout.decode("utf-8").strip()
        if Path(inside).resolve() == root.resolve():
            return sorted(name for name in listed.split("\0") if name and (root / name).is_file())
    except (OSError, subprocess.CalledProcessError):
        pass
    return sorted(
        path.relative_to(root).as_posix()
        for path in root.rglob("*") if path.is_file() and ".git" not in path.parts
    )


def imports(root: Path, name: str) -> list[str]:
    text = CODE_SPAN.sub("", FENCE.sub("", read(root / name)))
    found = []
    for match in IMPORT.finditer(text):
        target = (match.group(1) or match.group(2)).rstrip(".,;:").replace("\\", "/")
        for base in ((root / name).parent, root):
            candidate = (base / target).resolve()
            if candidate.is_file() and root.resolve() in candidate.parents:
                found.append(candidate.relative_to(root.resolve()).as_posix())
                break
    return found


def closure(root: Path, names: list[str]) -> list[str]:
    seen: list[str] = []
    queue = list(names)
    while queue:
        name = queue.pop(0)
        if name not in seen:
            seen.append(name)
            queue.extend(imports(root, name))
    return seen


def auto_loaded_sets(root: Path, files: list[str]) -> dict[str, list[str]]:
    sets = {"the core alone": closure(root, [CORE])}
    for tool, places in TOOL_FILES.items():
        own = [
            name for name in files
            if any(name == place or (place.endswith("/") and name.startswith(place))
                   for place in places)
        ]
        if own:
            sets[tool] = closure(root, own)
    return sets


def index_entries(core_text: str) -> dict[str, str]:
    """Each module the core's index names, with the header its module must open with."""
    entries: dict[str, str] = {}
    for line in core_text.split("\n"):
        match = INDEX_ROW.match(line)
        if not match:
            continue
        name = match.group(1)
        if name in entries:
            raise CheckError(f"{CORE}: the index names {name} twice")
        cells = [squeeze(cell) for cell in match.group(2).split("|")]
        if len(cells) != len(INDEX_COLUMNS) or not all(cells):
            raise CheckError(
                f"{CORE}: the index row for {name} needs a filled cell for each of "
                + ", ".join(INDEX_COLUMNS)
            )
        scope, task, paths, actions = cells
        entries[name] = (f"Scope: {scope} Read in full when: Task -- {task} "
                         f"Paths -- {paths} Actions -- {actions}")
    return entries


def module_header(text: str) -> list[str]:
    """The paragraphs between a module's title and its first section."""
    lines = text.split("\n")
    if not lines or not lines[0].startswith("# "):
        raise CheckError("does not open with a title")
    body: list[str] = []
    for line in lines[1:]:
        if line.startswith("#"):
            break
        body.append(line)
    return [squeeze(part) for part in "\n".join(body).split("\n\n") if part.strip()]


def path_triggers(entry: str) -> list[str]:
    clause = PATHS_CLAUSE.search(entry)
    return CODE_SPAN.findall(clause.group(1)) if clause else []


def declared_budget(core_text: str) -> int:
    declared = BUDGET.findall(core_text)
    if len(declared) != 1:
        raise CheckError(
            f"{CORE}: expected one 'Budget: **<n>** bytes' declaration, found {len(declared)}"
        )
    return int(declared[0].replace(",", ""))


def check(root: Path) -> list[str]:
    """Return the report lines; raise CheckError naming every failure."""
    if not (root / CORE).is_file():
        raise CheckError(f"{CORE} is missing")
    core_text = read(root / CORE)
    budget = declared_budget(core_text)
    files = tracked_files(root)
    failures: list[str] = []
    report: list[str] = []

    modules = sorted(name for name in files if name.startswith(MODULE_DIR + "/"))
    entries = index_entries(core_text)
    for name in modules:
        if not name.endswith(".md"):
            failures.append(f"{name}: only modules live under {MODULE_DIR}/")
        elif name not in entries:
            failures.append(f"{name}: a module the index in {CORE} does not name")
    for name in entries:
        if name not in modules:
            failures.append(f"{CORE}: the index names {name}, which does not exist")

    for name, entry in entries.items():
        if name not in modules:
            continue
        try:
            paragraphs = module_header(read(root / name))
        except CheckError as error:
            failures.append(f"{name}: {error}")
            continue
        joined = [" ".join(paragraphs[:count]) for count in range(1, len(paragraphs) + 1)]
        if entry not in joined:
            failures.append(
                f"{name}: does not open with its scope and triggers as the index in {CORE} "
                "gives them, word for word"
            )

    for name in modules:
        size = loaded_bytes(root / name)
        report.append(f"module {name}: {size:,} of {budget:,} bytes ({size * 100 // budget}%)")
        if size > budget:
            failures.append(f"{name}: {size:,} bytes exceeds the module budget of {budget:,}")

    for label, names in auto_loaded_sets(root, files).items():
        size = sum(loaded_bytes(root / name) for name in names)
        report.append(
            f"loaded by {label}: {' + '.join(names)} = {size:,} of {budget:,} bytes "
            f"({size * 100 // budget}%)"
        )
        if size > budget:
            failures.append(
                f"{label} loads {size:,} bytes ({', '.join(names)}), over the budget of {budget:,}"
            )
        for name in names:
            if name.startswith(MODULE_DIR + "/"):
                failures.append(
                    f"{label} loads the module {name} on every task: a stub or the core imports it"
                )

    known = {CORE} | {place for places in TOOL_FILES.values() for place in places}
    for name in files:
        if name.rsplit("/", 1)[-1] in RULE_FILE_NAMES and name not in known:
            failures.append(f"{name}: a rule file a tool may load, outside the core and its stubs")

    line_target = LINES.search(core_text)
    if line_target:
        lines = core_text.count("\n")
        report.append(f"{CORE}: {lines} of {line_target.group(1)} lines")
        if lines > int(line_target.group(1)):
            failures.append(f"{CORE}: {lines} lines, over the {line_target.group(1)} it declares")

    if failures:
        raise CheckError("\n".join(failures))
    return report


def triggered(root: Path, base: str) -> list[str]:
    """Modules whose path triggers the branch's changed files fire."""
    def git(*args: str) -> list[str]:
        result = subprocess.run(["git", "-C", str(root), *args], capture_output=True)
        if result.returncode != 0:
            raise CheckError(result.stderr.decode("utf-8", "replace").strip())
        return [line for line in result.stdout.decode("utf-8").splitlines() if line]

    changed = sorted(set(
        git("diff", "--name-only", f"{base}...HEAD")
        + git("diff", "--name-only", "HEAD")
        + git("ls-files", "--others", "--exclude-standard")
    ))
    lines = [f"{len(changed)} files changed against {base}, the working tree included"]
    for name, entry in index_entries(read(root / CORE)).items():
        hits = fired(path_triggers(entry), changed)
        if hits:
            shown = ", ".join(hits[:3]) + (f" and {len(hits) - 3} more" if len(hits) > 3 else "")
            lines.append(f"read {name}: {shown}")
    lines.append("path triggers only: check each module's task and action triggers by hand")
    return lines


def fired(triggers: list[str], changed: list[str]) -> list[str]:
    """A trigger is a file, a directory ending in `/`, or a pattern with `*`."""
    places = [trigger.strip("`") for trigger in triggers]
    return [
        name for name in changed
        if any(name == place or (place.endswith("/") and name.startswith(place))
               or ("*" in place and fnmatch.fnmatchcase(name, place))
               for place in places)
    ]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2],
                        help="the tree to check (default: this repository)")
    parser.add_argument("--triggered", action="store_true",
                        help="list the modules whose path triggers the branch's changes fire")
    parser.add_argument("--base", default="master",
                        help="the revision --triggered compares against (default: master)")
    args = parser.parse_args()
    try:
        lines = triggered(args.root, args.base) if args.triggered else check(args.root)
    except CheckError as error:
        for line in str(error).splitlines():
            print(f"FAIL: {line}", file=sys.stderr)
        return 1
    for line in lines:
        print(line)
    if not args.triggered:
        print("agent rules: within budget, index and modules agree")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
