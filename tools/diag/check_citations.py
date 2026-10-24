#!/usr/bin/env python3
"""Every commit a document cites must stay reachable.

A tracked Markdown file (outside the verbatim snapshots in docs/reference/)
cites a commit as a backticked hexadecimal hash of 7 to 40 characters. Each
must resolve to a commit reachable here: on `master`, under a pull request
(GitHub keeps `refs/pull/<n>/head` after a squash merge) or on a kept tag. A
hash that names something else -- another repository's commit, or a content
hash -- is declared once in tools/diag/citation_exceptions.tsv with its kind
and reason, and the check fails on an undeclared unresolved hash or on a
declaration nothing cites any more.

The pull-request refs are not fetched by default. Fetch them once:

    git fetch origin "+refs/pull/*/head:refs/remotes/origin/pr/*"

Exit status 0 when every citation resolves or is declared; 1 otherwise.
"""
from __future__ import annotations

import collections
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EXCEPTIONS = ROOT / "tools" / "diag" / "citation_exceptions.tsv"
CITATION = re.compile(r"`([0-9a-f]{7,40})`")
KINDS = {"external", "content", "lost"}


def cited_hashes(root: Path) -> dict[str, set[str]]:
    listed = subprocess.run(["git", "ls-files", "*.md"], cwd=root, check=True,
                            capture_output=True, text=True).stdout.split()
    cites: dict[str, set[str]] = collections.defaultdict(set)
    for name in listed:
        if name.startswith("docs/reference/"):
            continue
        text = (root / name).read_text(encoding="utf-8")
        for match in CITATION.finditer(text):
            token = match.group(1)
            if not token.isdigit():          # a number such as a node count
                cites[token].add(name)
    return cites


def unresolved(root: Path, hashes: list[str]) -> set[str]:
    query = "".join(f"{h}^{{commit}}\n" for h in hashes)
    lines = subprocess.run(["git", "cat-file", "--batch-check"], cwd=root, input=query,
                           capture_output=True, text=True, check=True).stdout.splitlines()
    return {h for h, line in zip(hashes, lines) if line.endswith((" missing", " ambiguous"))}


def declared(path: Path) -> dict[str, tuple[str, str]]:
    out: dict[str, tuple[str, str]] = {}
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line.strip() or line.startswith("#"):
            continue
        fields = line.split("\t")
        if len(fields) != 3 or fields[1] not in KINDS or not fields[2].strip():
            raise ValueError(f"{path.name}:{number}: expected hash<TAB>kind<TAB>reason, "
                             f"kind one of {sorted(KINDS)}")
        out[fields[0]] = (fields[1], fields[2])
    return out


def main() -> int:
    pr_refs = subprocess.run(["git", "for-each-ref", "--count=1", "refs/remotes/origin/pr/"],
                             cwd=ROOT, capture_output=True, text=True).stdout.strip()
    if not pr_refs:
        print("check_citations: no pull-request refs; fetch them first:\n"
              '  git fetch origin "+refs/pull/*/head:refs/remotes/origin/pr/*"', file=sys.stderr)
        return 1
    cites = cited_hashes(ROOT)
    missing = unresolved(ROOT, sorted(cites))
    try:
        exceptions = declared(EXCEPTIONS)
    except ValueError as error:
        print(f"check_citations: {error}", file=sys.stderr)
        return 1
    failures = []
    for token in sorted(missing - exceptions.keys()):
        failures.append(f"unreachable commit `{token}` cited in {', '.join(sorted(cites[token]))}")
    for token in sorted(exceptions.keys() - cites.keys()):
        failures.append(f"declared exception `{token}` is no longer cited; remove it")
    for token in sorted(exceptions.keys() & cites.keys() - missing):
        if exceptions[token][0] != "external":
            failures.append(f"declared `{token}` as {exceptions[token][0]} but it resolves here")
    if failures:
        print("\n".join(f"check_citations: {line}" for line in failures), file=sys.stderr)
        return 1
    print(f"citations: {len(cites)} cited hashes, {len(cites) - len(missing)} reachable, "
          f"{len(missing)} declared exceptions")
    return 0


if __name__ == "__main__":
    sys.exit(main())
