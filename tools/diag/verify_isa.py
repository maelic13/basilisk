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


# x86 objdump prints the bytes in pairs; AArch64 prints one 32-bit word.
INSTRUCTION = re.compile(
    r"^\s*[0-9a-f]+:\s+(?:(?:[0-9a-f]{2}\s+)+|[0-9a-f]{8}\s+)([a-z][a-z0-9.]*)\b",
    re.IGNORECASE,
)
# The ARM64 assets target baseline ARMv8 with NEON. These extensions are not
# guaranteed there, so none of them may appear.
ARM64_FORBIDDEN = {
    "sve": r"ptrues?|whilel[eost]|ptest|cnt[bhwd]|inc[bhwd]|dec[bhwd]|ld1[bhwd]|st1[bhwd]|ld1r[bhwd]",
    "dotprod": r"[su]dot|usdot|sudot",
    "aes": r"aes(?:e|d|mc|imc)",
    "sha": r"sha(?:1[chmp]|1su[01]|256h2?|256su[01]|512h2?|512su[01])",
}
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


SYMBOL = re.compile(r"^[0-9a-f]+ <(.+)>:\s*$", re.IGNORECASE)

# Library helpers whose extension instructions run only after a runtime check
# for that extension, so they are allowed on the ARM64 baseline. Each entry
# names the function, never a pattern: any other occurrence still fails.
# compiler-rt's SME support routine reads the vector length with SVE's cntd
# only when the CPU reports SVE; Windows links compiler-rt, Linux libgcc.
ARM64_RUNTIME_GATED = {"__arm_get_current_vg"}


def instructions(text: str) -> list[tuple[str, str, str]]:
    """(enclosing symbol, mnemonic, line) for every instruction line."""
    symbol = ""
    found = []
    for line in text.splitlines():
        label = SYMBOL.match(line)
        if label:
            symbol = label.group(1)
            continue
        match = INSTRUCTION.match(line)
        if match:
            found.append((symbol, match.group(1).lower(), line.strip()))
    return found


def instruction_counts(text: str) -> Counter[str]:
    return Counter(mnemonic for _, mnemonic, _ in instructions(text))


def arm64_forbidden(mnemonic: str) -> str | None:
    for family, pattern in ARM64_FORBIDDEN.items():
        if re.fullmatch(pattern, mnemonic):
            return family
    return None


def arm64_split(text: str) -> tuple[Counter[str], list[str], list[str]]:
    """Counts with runtime-gated helpers' extension instructions left out,
    plus the allowed and the offending occurrences, each named by function."""
    counts: Counter[str] = Counter()
    allowed, offending = [], []
    for symbol, mnemonic, line in instructions(text):
        family = arm64_forbidden(mnemonic)
        if family and symbol in ARM64_RUNTIME_GATED:
            allowed.append(f"{family} in <{symbol}>: {line}")
            continue
        if family:
            offending.append(f"{family} in <{symbol or '?'}>: {line}")
        counts[mnemonic] += 1
    return counts, allowed, offending


def family_count(counts: Counter[str], pattern: str) -> int:
    regex = re.compile(pattern)
    return sum(count for mnemonic, count in counts.items() if regex.fullmatch(mnemonic))


def verify_binary(tier: str, counts: Counter[str]) -> tuple[dict[str, int], list[str]]:
    if tier == "arm64":
        observed = {"prfm": family_count(counts, r"prfm")}
        observed.update({name: family_count(counts, pattern) for name, pattern in ARM64_FORBIDDEN.items()})
        errors = []
        if not observed["prfm"]:
            # The transposition table's prefetch compiles to prfm; without it the
            # hint is silently gone, which cost Rarog three releases of speed.
            errors.append("ARM64 tier contains no prfm instruction: the TT prefetch is missing")
        for family in ARM64_FORBIDDEN:
            if observed[family]:
                errors.append(f"ARM64 tier contains forbidden {family} instructions")
        return observed, errors
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
    parser.add_argument("--tier", required=True, choices=("portable", "avx2", "pext", "arm64"))
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
        if args.tier == "arm64":
            counts, allowed, offending = arm64_split(binary_text)
            for entry in allowed:
                print(f"allowed (runtime-gated library helper): {entry}")
            for entry in offending[:20]:
                print(f"forbidden: {entry}", file=sys.stderr)
        else:
            counts = instruction_counts(binary_text)
        observed, errors = verify_binary(args.tier, counts)
        print(
            f"{args.tier}: "
            + " ".join(f"{name}={count}" for name, count in observed.items())
        )

        if args.launcher_object is not None:
            launcher_text = disassemble(objdump, args.launcher_object, relocations=True)
            errors.extend(verify_launcher(args.tier, launcher_text))
        elif args.tier not in ("portable", "arm64"):
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
