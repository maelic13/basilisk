#!/usr/bin/env python3
"""Release checks for the tag-driven release flow (`.github/workflows/release.yml`).

  check vX.Y.Z [--base REF] [--notes FILE]
      The tag names the version both sources declare (`CMakeLists.txt`'s
      `project(... VERSION X.Y.Z)` and `src/constants.h`'s `engineVersion`);
      CHANGELOG.md has a dated, non-empty `## [X.Y.Z] - YYYY-MM-DD` section;
      GUIDE's Released baseline row names X.Y.Z (the release commit marks
      itself released); HEAD is on the release line (see `line`). With
      --notes, the CHANGELOG section is written there as release notes.
  line vX.Y.Z [--base REF]
      HEAD is an ancestor of REF (default `origin/master`), or the tag is a
      patch (Z > 0) whose commit descends from the `vX.Y.0` tag on REF: a
      patch cut on a branch from the release while REF holds later work.
  fingerprint
      Prints the `bench 13` node count GUIDE declares, read by the roadmap
      checker's own parser, so the workflow and the checker cannot disagree.
  version
      Prints `engineVersion` (X.Y.Z, or X.Y.Z-dev between releases) after
      checking that its numeric part equals CMake's project version.

Exit status 0 on success; 1 with the reason on stderr otherwise.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_roadmap  # noqa: E402  (the single fingerprint parser)

ROOT = Path(__file__).resolve().parents[2]
TAG = re.compile(r"v(\d+\.\d+\.\d+)")
ENGINE_VERSION = re.compile(r'engineVersion\s*=\s*"(\d+\.\d+\.\d+)(-dev)?"')
CMAKE_VERSION = re.compile(r"^project\(basilisk VERSION (\d+\.\d+\.\d+)\b", re.MULTILINE)
DATE = re.compile(r"\d{4}-\d{2}-\d{2}")
BOLD_VERSION = re.compile(r"\*\*(\d+\.\d+\.\d+)\*\*")


class CheckError(Exception):
    pass


def version_of_tag(tag: str) -> str:
    match = TAG.fullmatch(tag)
    if not match:
        raise CheckError(f"tag `{tag}` must read vX.Y.Z with three numbers")
    return match.group(1)


def engine_version(root: Path) -> str:
    """engineVersion, checked against CMake's numeric project version."""
    constants = (root / "src" / "constants.h").read_text(encoding="utf-8")
    match = ENGINE_VERSION.search(constants)
    if not match:
        raise CheckError("src/constants.h declares no engineVersion \"X.Y.Z\" or \"X.Y.Z-dev\"")
    cmake = CMAKE_VERSION.search((root / "CMakeLists.txt").read_text(encoding="utf-8"))
    if not cmake:
        raise CheckError("CMakeLists.txt declares no `project(basilisk VERSION X.Y.Z`")
    if cmake.group(1) != match.group(1):
        raise CheckError(f"CMakeLists.txt declares {cmake.group(1)}, "
                         f"src/constants.h {match.group(1)}{match.group(2) or ''}")
    return match.group(1) + (match.group(2) or "")


def changelog_section(changelog: str, version: str) -> str:
    lines = changelog.splitlines()
    heading = f"## [{version}] - "
    start = next((n for n, line in enumerate(lines)
                  if line.startswith(heading) and DATE.fullmatch(line[len(heading):].strip())), None)
    if start is None:
        raise CheckError(f"CHANGELOG.md has no `## [{version}] - YYYY-MM-DD` section; "
                         "date [Unreleased] as the release first")
    end = next((n for n in range(start + 1, len(lines)) if lines[n].startswith("## ")), len(lines))
    body = lines[start + 1:end]
    while body and body[-1].strip() in ("", "---"):
        body.pop()
    text = "\n".join(body).strip()
    if not text:
        raise CheckError(f"the CHANGELOG section for {version} is empty")
    return text + "\n"


def released_in_guide(guide: str, version: str) -> None:
    rows = [line for line in guide.splitlines() if line.startswith("| Released baseline |")]
    if len(rows) != 1:
        raise CheckError(f"GUIDE.md has {len(rows)} `| Released baseline |` rows; it needs one")
    named = BOLD_VERSION.search(rows[0])
    if not named:
        raise CheckError("GUIDE's Released baseline row names no **X.Y.Z** version")
    if named.group(1) != version:
        raise CheckError(f"GUIDE's Released baseline row names {named.group(1)}; "
                         f"the release commit must mark {version} released")


def declared_fingerprint(root: Path) -> int:
    try:
        return int(check_roadmap.validate_fingerprint(root).replace(",", ""))
    except ValueError as error:  # the checker reports its own reason
        raise CheckError(str(error)) from error


def is_ancestor(root: Path, ancestor: str, descendant: str) -> bool:
    result = subprocess.run(["git", "merge-base", "--is-ancestor", ancestor, descendant],
                            cwd=root, capture_output=True, text=True)
    if result.returncode in (0, 1):
        return result.returncode == 0
    raise CheckError(f"cannot compare {ancestor} with {descendant}: {result.stderr.strip()}")


def tag_exists(root: Path, tag: str) -> bool:
    result = subprocess.run(["git", "rev-parse", "--verify", "--quiet", f"refs/tags/{tag}^{{commit}}"],
                            cwd=root, capture_output=True, text=True)
    return result.returncode == 0


def ensure_on_line(root: Path, version: str, base: str) -> str:
    """HEAD is on `base`, or, for a patch X.Y.Z with Z > 0, HEAD descends from
    the `vX.Y.0` tag and that tag is on `base`: a patch released while `base`
    already holds later work is built on a branch from the release."""
    if is_ancestor(root, "HEAD", base):
        return f"HEAD on {base}"
    major, minor, patch = version.split(".")
    if patch == "0":
        raise CheckError(f"HEAD is not on {base}")
    line = f"v{major}.{minor}.0"
    if not tag_exists(root, line):
        raise CheckError(f"HEAD is not on {base}, and the patch line's {line} tag does not exist")
    if not is_ancestor(root, line, base):
        raise CheckError(f"HEAD is not on {base}, and {line} is not on {base} either")
    if not is_ancestor(root, line, "HEAD"):
        raise CheckError(f"HEAD is not on {base} and does not descend from {line}")
    return f"HEAD on the {major}.{minor} patch line from {line} on {base}"


def check(root: Path, tag: str, base: str, notes: Path | None) -> str:
    version = version_of_tag(tag)
    declared = engine_version(root)
    if declared != version:
        raise CheckError(f"tag {tag} names {version}, but the sources declare {declared}")
    body = changelog_section((root / "CHANGELOG.md").read_text(encoding="utf-8"), version)
    released_in_guide((root / "GUIDE.md").read_text(encoding="utf-8"), version)
    nodes = declared_fingerprint(root)
    where = ensure_on_line(root, version, base)
    if notes is not None:
        notes.write_text(body, encoding="utf-8")
    return (f"release check {tag}: both version sources read {version}, CHANGELOG section "
            f"dated, GUIDE marks it released, bench 13 declared {nodes}, {where}")


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    run = sub.add_parser("check")
    run.add_argument("tag")
    run.add_argument("--base", default="origin/master")
    run.add_argument("--notes", type=Path)
    line = sub.add_parser("line")
    line.add_argument("tag")
    line.add_argument("--base", default="origin/master")
    sub.add_parser("fingerprint")
    sub.add_parser("version")
    args = parser.parse_args(argv)
    try:
        if args.command == "check":
            print(check(ROOT, args.tag, args.base, args.notes))
        elif args.command == "line":
            print(f"release line {args.tag}: "
                  + ensure_on_line(ROOT, version_of_tag(args.tag), args.base))
        elif args.command == "fingerprint":
            print(declared_fingerprint(ROOT))
        else:
            print(engine_version(ROOT))
    except CheckError as error:
        print(f"release_check: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
