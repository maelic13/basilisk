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


SYMBOL = re.compile(r"^([0-9a-f]+) <(.+)>:\s*$", re.IGNORECASE)

# Library helpers whose extension instructions run only after a runtime check
# for that extension, so they are allowed on the ARM64 baseline. Each entry
# names the function, never a pattern: any other occurrence still fails.
# LLVM libunwind's Registers_arm64::getRegister reads the vector-granule
# register with SVE's cntd only when it unwinds through a frame whose unwind
# information names that register, which only SVE code produces; the engine has
# none. Windows links LLVM libunwind, Linux libgcc's unwinder, so only the
# Windows assets carry it (found by PR #9's first runs).
ARM64_RUNTIME_GATED = {"_ZNK9libunwind15Registers_arm6411getRegisterEi"}

# A statically linked Linux binary also carries the C runtime that a dynamic
# one loads from shared libraries: glibc's string and math variants, which its
# IFUNC resolvers pick by CPU at startup (__memcpy_a64fx, __strlen_evex), and
# libgcc's unwinder, which reads SVE state only for SVE frames. Functions the
# runtime archives named by --runtime-archive define are left out, so the
# contract covers what a dynamic build's binary held. nm's symbol types for
# code: T/t text, W/w weak, i indirect (IFUNC).
CODE_SYMBOL_TYPES = set("TtWwi")


def archive_functions(nm: str, archive: Path) -> set[str]:
    # glibc's libm.a is a linker script naming the real archives.
    if not archive.read_bytes()[:8] == b"!<arch>\n":
        script = re.sub(r"/\*.*?\*/", "", archive.read_text(), flags=re.DOTALL)
        names = set()
        for group in re.findall(r"(?:GROUP|INPUT)\s*\(([^)]*)\)", script):
            for member in group.replace("AS_NEEDED", " ").replace("(", " ").split():
                path = Path(member) if Path(member).is_absolute() else archive.parent / member
                names |= archive_functions(nm, path)
        if not names:
            raise RuntimeError(f"{archive} is neither an archive nor a linker script naming one")
        return names
    result = subprocess.run([nm, "--defined-only", str(archive)],
                            check=False, capture_output=True, text=True)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or f"{nm} exited {result.returncode}")
    names = set()
    for line in result.stdout.splitlines():
        fields = line.split()
        if len(fields) == 3 and fields[1] in CODE_SYMBOL_TYPES:
            names.add(fields[2])
    if not names:
        raise RuntimeError(f"{archive} defines no functions")
    return names


def instructions(text: str, exempt: frozenset[str] = frozenset()) -> list[tuple[str, str, str]]:
    """(enclosing symbol, mnemonic, line) for every instruction line outside
    the exempt functions."""
    symbol = ""
    found = []
    for line in text.splitlines():
        label = SYMBOL.match(line)
        if label:
            symbol = label.group(2)
            continue
        if symbol in exempt:
            continue
        match = INSTRUCTION.match(line)
        if match:
            found.append((symbol, match.group(1).lower(), line.strip()))
    return found


def instruction_counts(text: str, exempt: frozenset[str] = frozenset()) -> Counter[str]:
    return Counter(mnemonic for _, mnemonic, _ in instructions(text, exempt))


def arm64_forbidden(mnemonic: str) -> str | None:
    for family, pattern in ARM64_FORBIDDEN.items():
        if re.fullmatch(pattern, mnemonic):
            return family
    return None


def arm64_split(text: str, exempt: frozenset[str] = frozenset()) -> tuple[Counter[str], list[str], list[str]]:
    """Counts with runtime-gated helpers' extension instructions left out,
    plus the allowed and the offending occurrences, each named by function."""
    counts: Counter[str] = Counter()
    allowed, offending = [], []
    for symbol, mnemonic, line in instructions(text, exempt):
        family = arm64_forbidden(mnemonic)
        if family and symbol in ARM64_RUNTIME_GATED:
            allowed.append(f"{family} in <{symbol}>: {line}")
            continue
        if family:
            offending.append(f"{family} in <{symbol or '?'}>: {line}")
        counts[mnemonic] += 1
    return counts, allowed, offending


# Everything above the portable floor, by family. The portable tier and the
# code every universal tier shares contain none of it; the avx2 tier none of
# BMI1, BMI2 or LZCNT, which its CPUs need not have.
# - The floor is x86-64 with SSE3: MSYS2 builds its static libc++ for
#   -march=nocona, and every x86-64 CPU since 2005 has SSE3. SSSE3 is not on
#   it (AMD's K10, Phenom II and Athlon II, lacks it).
# - tzcnt is allowed: it is LLVM's encoding of bsf (f3 0f bc, a rep-prefixed
#   bsf) for a count of trailing zeros whose input is not zero, and a CPU
#   without BMI1 runs it as bsf with the same result.
X86_ABOVE_BASELINE = {
    "popcnt": r"popcnt[qlw]?",
    "lzcnt": r"lzcnt[qlw]?",
    "bmi1": r"(?:andn|bextr|blsi|blsmsk|blsr)[qlw]?",
    "bmi2": r"(?:bzhi|mulx|pdep|pext|rorx|sarx|shlx|shrx)[qlw]?",
    "ssse3": r"pshufb|palignr|ph(?:add|sub)(?:w|d|sw)|pabs[bwd]|psign[bwd]|pmaddubsw|pmulhrsw",
    "sse4.1": r"blendv?p[sd]|pblend(?:w|vb)|dpp[sd]|(?:insert|extract)ps|pextr[bdq]|pinsr[bdq]"
              r"|pm(?:ax|in)(?:sb|sd|ud|uw)|pmov[sz]x(?:bw|bd|bq|wd|wq|dq)|pmul(?:dq|ld)|ptest"
              r"|round[ps][sd]|mpsadbw|packusdw|pcmpeqq|phminposuw|movntdqa",
    "sse4.2": r"pcmpgtq|pcmp[ei]str[im]|crc32[bwlq]?",
    # Every VEX or EVEX encoding: AVX and above (verr and verw are not).
    "avx": r"v(?!err|erw)[a-z0-9]+",
}
AVX2_FORBIDDEN = ("lzcnt", "bmi1", "bmi2")


def above_baseline(mnemonic: str) -> str | None:
    for family, pattern in X86_ABOVE_BASELINE.items():
        if re.fullmatch(pattern, mnemonic):
            return family
    return None


def x86_offenders(text: str, exempt: frozenset[str] = frozenset(),
                  families: tuple[str, ...] = tuple(X86_ABOVE_BASELINE)) -> Counter[str]:
    """Functions holding instructions of the given families, with counts."""
    return Counter(symbol or "?" for symbol, mnemonic, _ in instructions(text, exempt)
                   if above_baseline(mnemonic) in families)


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
    families = Counter()
    for mnemonic, count in counts.items():
        family = above_baseline(mnemonic)
        if family:
            families[family] += count
    errors: list[str] = []

    if tier == "portable":
        for family in X86_ABOVE_BASELINE:
            if families[family]:
                errors.append(f"portable tier contains forbidden {family} instructions")
        observed.update({name: families[name] for name in ("bmi1", "sse4.1", "avx")})
    elif tier == "avx2":
        for family in ("popcnt", "avx2"):
            if not observed[family]:
                errors.append(f"AVX2 tier contains no {family} instruction")
        for family in AVX2_FORBIDDEN:
            if families[family]:
                errors.append(f"AVX2 tier contains forbidden {family} instructions")
    elif tier == "pext":
        for family in ("popcnt", "pext", "blsr", "lzcnt", "avx2"):
            if not observed[family]:
                errors.append(f"PEXT tier contains no {family} instruction")

    return observed, errors


# A universal binary's tiers: every symbol a tier defines carries its name
# (cmake/universal-tiers.cmake), so each function is attributed by its label.
# Everything else (the dispatcher, Fathom, the runtime) is shared and runs on
# every CPU, so it meets the portable contract, as the base tier does.
UNIVERSAL_TIERS = ("base", "avx2", "pext")
TIER_SYMBOL = re.compile(r"__basilisk_(base|avx2|pext)_|_ZN\d+basilisk_(base|avx2|pext)10run_engine")


def symbol_region(symbol: str) -> str:
    match = TIER_SYMBOL.match(symbol)
    return (match.group(1) or match.group(2)) if match else "shared"


# Each tier's functions are aligned to 64 bytes, so the relink cannot move them
# off the cache-line placement they had in the tier's own build (BAS-P20 read
# that shift 0.58% slower). Compiler-generated stubs (destructor thunks,
# terminate handlers) can escape the flag, so the rule is a share of each tier
# plus every function of the hot path, by mangled name.
ALIGNMENT = 64
ALIGNED_SHARE = 0.95
HOT_FUNCTIONS = (
    "N8Searcher7negamax", "N8Searcher10quiescence", "N14CoreMovePicker4next",
    "N5Board9make_move", "N5Board11unmake_move", "NK5Board8is_legal", "NK5Board6see_ge",
    "N9Evaluator8evaluate",
)
# Fathom is compiled once, for the baseline, and linked once beside the
# dispatcher: no tier may carry its own copy.
FATHOM_FUNCTIONS = ("tb_init", "tb_free", "tb_probe_wdl_impl", "tb_probe_root_impl")


def functions(text: str) -> list[tuple[int, str]]:
    """(address, symbol) for every function label in the disassembly."""
    return [(int(m.group(1), 16), m.group(2)) for m in map(SYMBOL.match, text.splitlines()) if m]


def verify_layout(text: str) -> list[str]:
    errors: list[str] = []
    found = functions(text)
    for tier in UNIVERSAL_TIERS:
        own = [(address, symbol) for address, symbol in found if symbol_region(symbol) == tier]
        if not own:
            continue
        aligned = sum(1 for address, _ in own if address % ALIGNMENT == 0)
        print(f"{tier}: {aligned} of {len(own)} functions at 0 mod {ALIGNMENT}")
        if aligned < ALIGNED_SHARE * len(own):
            errors.append(f"{tier}: only {aligned} of {len(own)} functions start on a {ALIGNMENT}-byte line")
        for hot in HOT_FUNCTIONS:
            matches = [(address, symbol) for address, symbol in own if hot in symbol]
            if not matches:
                errors.append(f"{tier}: no function matching the hot-path name {hot}")
            for address, symbol in matches:
                if address % ALIGNMENT:
                    errors.append(f"{tier}: hot function {symbol} starts at {address % ALIGNMENT} mod {ALIGNMENT}")
    for name in FATHOM_FUNCTIONS:
        shared = [symbol for _, symbol in found if symbol == name]
        in_tiers = [symbol for _, symbol in found if symbol_region(symbol) != "shared" and symbol.endswith("_" + name)]
        if len(shared) != 1 or in_tiers:
            errors.append(f"Fathom's {name} is defined {len(shared)} times as shared code and in tiers as {in_tiers}")
    return errors


def verify_universal(text: str, exempt: frozenset[str]) -> list[str]:
    regions: dict[str, Counter[str]] = {name: Counter() for name in ("shared", *UNIVERSAL_TIERS)}
    offenders: Counter[str] = Counter()
    for symbol, mnemonic, _ in instructions(text, exempt):
        region = symbol_region(symbol)
        regions[region][mnemonic] += 1
        family = above_baseline(mnemonic)
        if family and (region in ("shared", "base")
                       or (region == "avx2" and family in AVX2_FORBIDDEN)):
            offenders[f"{region} <{symbol or '?'}> {family}"] += 1
    errors: list[str] = []
    for region, counts in regions.items():
        if not counts:
            errors.append(f"universal binary holds no {region} code")
            continue
        observed, region_errors = verify_binary("portable" if region in ("shared", "base") else region, counts)
        errors.extend(f"{region}: {error}" for error in region_errors)
        print(f"{region}: " + " ".join(f"{name}={count}" for name, count in observed.items()))
    for entry, count in offenders.most_common(20):
        print(f"forbidden: {count} in {entry}", file=sys.stderr)
    errors.extend(verify_layout(text))
    return errors


def verify_launcher(tier: str, text: str) -> list[str]:
    counts = instruction_counts(text)
    observed, errors = verify_binary("portable", counts)
    errors = [f"launcher: {error}" for error in errors]

    entries = ([f"basilisk_{name}10run_engine" for name in UNIVERSAL_TIERS]
               if tier == "universal" else ["run_engine"])
    for entry in entries:
        if entry not in text:
            errors.append(f"launcher: no relocation or symbol reference to {entry}")
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
    parser.add_argument("--tier", required=True, choices=("portable", "avx2", "pext", "universal", "arm64"))
    parser.add_argument("--binary", required=True, type=Path)
    parser.add_argument("--launcher-object", type=Path)
    parser.add_argument("--objdump", help="llvm-objdump executable")
    parser.add_argument("--runtime-archive", type=Path, action="append", default=[],
                        help="a statically linked C runtime archive whose functions are exempt")
    parser.add_argument("--nm", help="nm executable for --runtime-archive")
    args = parser.parse_args()

    objdump = args.objdump or shutil.which("llvm-objdump")
    if not objdump:
        parser.error("llvm-objdump was not found; pass --objdump")
    if not args.binary.is_file():
        parser.error(f"binary not found: {args.binary}")
    if args.launcher_object is not None and not args.launcher_object.is_file():
        parser.error(f"launcher object not found: {args.launcher_object}")

    nm = args.nm or shutil.which("llvm-nm") or shutil.which("nm")
    if args.runtime_archive and not nm:
        parser.error("nm was not found; pass --nm")
    for archive in args.runtime_archive:
        if not archive.is_file():
            parser.error(f"runtime archive not found: {archive}")

    try:
        exempt: set[str] = set()
        for archive in args.runtime_archive:
            names = archive_functions(nm, archive)
            print(f"exempt: {len(names)} functions of {archive}")
            exempt |= names
        binary_text = disassemble(objdump, args.binary)
        if args.tier == "arm64":
            counts, allowed, offending = arm64_split(binary_text, frozenset(exempt))
            for entry in allowed:
                print(f"allowed (runtime-gated library helper): {entry}")
            for entry in offending[:20]:
                print(f"forbidden: {entry}", file=sys.stderr)
        else:
            counts = instruction_counts(binary_text, frozenset(exempt))
        if args.tier == "universal":
            errors = verify_universal(binary_text, frozenset(exempt))
        else:
            observed, errors = verify_binary(args.tier, counts)
            if args.tier in ("portable", "avx2") and errors:
                families = tuple(X86_ABOVE_BASELINE) if args.tier == "portable" else AVX2_FORBIDDEN
                for symbol, count in x86_offenders(binary_text, frozenset(exempt), families).most_common(20):
                    print(f"forbidden: {count} in <{symbol}>", file=sys.stderr)
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
