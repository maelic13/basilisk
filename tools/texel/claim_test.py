#!/usr/bin/env python3
"""Claim a published corpus's frozen test split, once.

The test split of a basilisk-texel-rows-v1 corpus is read only after model
selection, for one candidate. A claim is a file, ``test.claim.json`` beside
the manifest, created with an exclusive create: two claims cannot both
succeed, however they race. It records the candidate, the purpose, the time
and the test file's hash. The claim is refused when one already exists, and
when the test file no longer matches the hash its manifest published.

    python tools/texel/claim_test.py claim DIR --candidate ID --purpose TEXT
    python tools/texel/claim_test.py status DIR

``claim`` prints the test file's path on success. Exit status: 0 claimed (or,
for ``status``, a claim exists), 1 refused or unclaimed, 2 a harness error.
"""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import os
import sys
from pathlib import Path

SCHEMA = "basilisk-texel-rows-v1"
CLAIM = "test.claim.json"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def load_manifest(corpus: Path) -> dict:
    manifest_path = corpus / "manifest.json"
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as exc:
        raise SystemExit(f"error: cannot read {manifest_path}: {exc}") from exc
    if manifest.get("schema") != SCHEMA:
        raise SystemExit(f"error: {manifest_path} is not a {SCHEMA} manifest")
    return manifest


def claim(corpus: Path, candidate: str, purpose: str) -> int:
    manifest = load_manifest(corpus)
    test = manifest["output"]["test"]
    test_path = corpus / test["file"]
    claim_path = corpus / CLAIM
    if claim_path.exists():
        print(f"refused: the test split is already claimed ({claim_path}):\n"
              f"{claim_path.read_text(encoding='utf-8')}", file=sys.stderr)
        return 1
    actual = sha256_file(test_path)
    if actual != test["sha256"]:
        print(f"refused: {test_path} hashes {actual}, its manifest published "
              f"{test['sha256']}", file=sys.stderr)
        return 1
    record = {
        "schema": SCHEMA,
        "candidate": candidate,
        "purpose": purpose,
        "claimed_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds"),
        "test_file": test["file"],
        "test_sha256": actual,
        "test_rows": test["rows"],
    }
    try:
        fd = os.open(claim_path, os.O_WRONLY | os.O_CREAT | os.O_EXCL)
    except FileExistsError:
        print(f"refused: the test split was claimed while this claim ran ({claim_path})",
              file=sys.stderr)
        return 1
    with os.fdopen(fd, "w", encoding="utf-8", newline="\n") as out:
        out.write(json.dumps(record, indent=2) + "\n")
    print(test_path)
    return 0


def status(corpus: Path) -> int:
    load_manifest(corpus)
    claim_path = corpus / CLAIM
    if not claim_path.exists():
        print("unclaimed")
        return 1
    print(claim_path.read_text(encoding="utf-8"), end="")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    claim_cmd = commands.add_parser("claim", help="claim the test split once")
    claim_cmd.add_argument("corpus", type=Path)
    claim_cmd.add_argument("--candidate", required=True,
                           help="the selected candidate, e.g. its vector's hash")
    claim_cmd.add_argument("--purpose", required=True)
    status_cmd = commands.add_parser("status", help="show the claim, if any")
    status_cmd.add_argument("corpus", type=Path)
    args = parser.parse_args()
    if args.command == "claim":
        if not args.candidate.strip() or not args.purpose.strip():
            parser.error("--candidate and --purpose must not be empty")
        return claim(args.corpus, args.candidate, args.purpose)
    return status(args.corpus)


if __name__ == "__main__":
    raise SystemExit(main())
