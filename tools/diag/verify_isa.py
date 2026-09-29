#!/usr/bin/env python3
"""Verify the executable CPU-tier contract from disassembly."""

from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import sys
from collections import Counter
from pathlib import Path


INSTRUCTION = re.compile(
    r"^\s*[0-9a-f]+:\s+(?:[0-9a-f]{2}\s+)+([a-z][a-z0-9.]*)\b",
    re.IGNORECASE,
)
AVX2_MNEMONICS = {
    "vextracti128",
    "vinserti128",
    "vpblendd",
    "vpbroadcastq",
    "vpermd",
    "vpermq",
    "vpmaskmovd",
    "vpmaskmovq",
}


def disassemble(objdump: str, path: Path, relocations: bool = False) -> str:
    args = [objdump, "-d"]
    if relocations:
        args.append("-r")
    args.append(str(path))
    result = subprocess.run(args, check=False, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or f"{objdump} exited {result.returncode}")
    return result.stdout


def instruction_counts(text: str) -> Counter[str]:
    counts: Counter[str] = Counter()
    for line in text.splitlines():
        match = INSTRUCTION.match(line)
        if match:
            counts[match.group(1).lower()] += 1
    return counts


def family_count(counts: Counter[str], pattern: str) -> int:
    regex = re.compile(pattern)
    return sum(count for mnemonic, count in counts.items() if regex.fullmatch(mnemonic))


def verify_binary(tier: str, counts: Counter[str]) -> tuple[dict[str, int], list[str]]:
    observed = {
        "popcnt": family_count(counts, r"popcnt[qlw]?"),
        "pext": family_count(counts, r"pext[qlwd]?"),
        "blsr": family_count(counts, r"blsr[qlwd]?"),
        "lzcnt": family_count(counts, r"lzcnt[qlw]?"),
        "avx2": sum(counts[mnemonic] for mnemonic in AVX2_MNEMONICS),
    }
    errors: list[str] = []

    if tier == "portable":
        for family in ("popcnt", "pext", "blsr", "lzcnt", "avx2"):
            if observed[family]:
                errors.append(f"portable tier contains forbidden {family} instructions")
    elif tier == "avx2":
        for family in ("popcnt", "avx2"):
            if not observed[family]:
                errors.append(f"AVX2 tier contains no {family} instruction")
        if observed["pext"]:
            errors.append("AVX2 tier contains forbidden PEXT instructions")
    elif tier == "pext":
        for family in ("popcnt", "pext", "blsr", "lzcnt", "avx2"):
            if not observed[family]:
                errors.append(f"PEXT tier contains no {family} instruction")

    return observed, errors


def verify_launcher(tier: str, text: str) -> list[str]:
    counts = instruction_counts(text)
    observed, errors = verify_binary("portable", counts)
    errors = [f"launcher: {error}" for error in errors]

    if "run_engine" not in text:
        errors.append("launcher: no relocation or symbol reference to run_engine")
    if tier != "portable" and not (
        "__cpu_model" in text or "cpuid" in counts
    ):
        errors.append("launcher: runtime CPU feature detection is absent or optimized away")

    print(
        "launcher: "
        + " ".join(f"{name}={count}" for name, count in observed.items())
    )
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tier", required=True, choices=("portable", "avx2", "pext"))
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--launcher-object", type=Path)
    parser.add_argument("--objdump", help="llvm-objdump executable")
    args = parser.parse_args()

    objdump = args.objdump or shutil.which("llvm-objdump")
    if not objdump:
        parser.error("llvm-objdump was not found; pass --objdump")
    if not args.binary.is_file():
        parser.error(f"binary not found: {args.binary}")
    if args.launcher_object is not None and not args.launcher_object.is_file():
        parser.error(f"launcher object not found: {args.launcher_object}")

    try:
        binary_text = disassemble(objdump, args.binary)
        observed, errors = verify_binary(args.tier, instruction_counts(binary_text))
        print(
            f"{args.tier}: "
            + " ".join(f"{name}={count}" for name, count in observed.items())
        )

        if args.launcher_object is not None:
            launcher_text = disassemble(objdump, args.launcher_object, relocations=True)
            errors.extend(verify_launcher(args.tier, launcher_text))
        elif args.tier != "portable":
            errors.append("specialized tiers require --launcher-object")
    except RuntimeError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2

    if errors:
        for error in errors:
            print(f"FAIL: {error}", file=sys.stderr)
        return 1

    print("ISA contract: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
