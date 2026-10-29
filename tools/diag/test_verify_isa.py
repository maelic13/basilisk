import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import verify_isa

# Lines as llvm-objdump prints them: x86 in byte pairs, AArch64 as one word.
X86 = """
   140001000: f3 48 0f b8 c7               popcntq %rdi, %rax
   140001005: c4 e2 c2 f5 c6               pextq  %rsi, %rdi, %rax
   14000100a: c3                           retq
"""
ARM64 = """
      10: f8a86800     \tprfm\tpldl1keep, [x0, x8]
      14: 4e829420     \tsdot\tv0.4s, v1.16b, v2.16b
      18: 2598e3e0     \tptrue\tp0.d
      1c: 4e284820     \taese\tv0.16b, v1.16b
      20: d65f03c0     \tret
"""


class FormatTests(unittest.TestCase):
    def test_both_objdump_formats_are_read(self):
        x86 = verify_isa.instruction_counts(X86)
        self.assertEqual((x86["popcntq"], x86["pextq"], x86["retq"]), (1, 1, 1))
        arm = verify_isa.instruction_counts(ARM64)
        self.assertEqual((arm["prfm"], arm["sdot"], arm["ptrue"], arm["aese"], arm["ret"]), (1, 1, 1, 1, 1))


class Arm64Tests(unittest.TestCase):
    def verify(self, text):
        return verify_isa.verify_binary("arm64", verify_isa.instruction_counts(text))

    def test_a_baseline_binary_with_its_prefetch_passes(self):
        observed, errors = self.verify(ARM64.split("\n")[1] + "\n" + ARM64.split("\n")[-2])
        self.assertEqual(errors, [])
        self.assertEqual(observed["prfm"], 1)

    def test_a_missing_prefetch_fails(self):
        _, errors = self.verify("      20: d65f03c0     \tret\n")
        self.assertTrue(any("no prfm" in e for e in errors))

    def test_each_forbidden_extension_fails(self):
        _, errors = self.verify(ARM64)
        for family in ("sve", "dotprod", "aes"):
            self.assertTrue(any(f"forbidden {family}" in e for e in errors), family)


# What each region of a clean universal image holds: the pext tier its five
# signature families, the avx2 tier popcnt and AVX2, the base tier and the
# shared code only baseline instructions.
TIER_CODE = {
    "pext": ["popcntq %rdi, %rax", "pextq %rsi, %rdi, %rax", "blsrq %rdi, %rax", "lzcntq %rdi, %rax",
             "vpermq $0x1, %ymm0, %ymm1"],
    "avx2": ["popcntq %rdi, %rax", "vpermq $0x1, %ymm0, %ymm1"],
    "base": ["movq %rdi, %rax"],
}


def universal_image(plant=None, misalign=None, extra_fathom=None):
    """A disassembly with every hot function of every tier, Fathom once and a
    dispatcher; plant=(region, instruction) adds one instruction to a region,
    misalign=(tier, hot) moves that function 16 bytes off its line."""
    lines, address = [], 0x140001000

    def function(name, body):
        nonlocal address
        lines.append(f"{address:016x} <{name}>:")
        for instruction in body + ["retq"]:
            lines.append(f"   {address:x}: 90 90               {instruction}")
        address += 64

    for tier, body in TIER_CODE.items():
        for hot in verify_isa.HOT_FUNCTIONS:
            name = f"__basilisk_{tier}__Z{hot}Ev"
            if misalign == (tier, hot):
                address += 16
            code = body + ([plant[1]] if plant and plant[0] == tier and hot == verify_isa.HOT_FUNCTIONS[0] else [])
            function(name, code)
            if misalign == (tier, hot):
                address -= 16
    shared = ["movq %rdi, %rax"] + ([plant[1]] if plant and plant[0] == "shared" else [])
    for name in verify_isa.FATHOM_FUNCTIONS:
        function(name, shared)
    if extra_fathom:
        function(f"__basilisk_pext_{extra_fathom}", ["movq %rdi, %rax"])
    function("main", ["cpuid"])
    return "\n".join(lines) + "\n"


class UniversalTests(unittest.TestCase):
    def errors(self, text):
        return verify_isa.verify_universal(text, frozenset())

    def test_a_clean_image_passes(self):
        self.assertEqual(self.errors(universal_image()), [])

    def test_a_planted_instruction_fails_in_its_region(self):
        for region, instruction, family in [("shared", "popcntq %rdi, %rax", "popcnt"),
                                            ("shared", "pextq %rsi, %rdi, %rax", "bmi2"),
                                            ("base", "popcntq %rdi, %rax", "popcnt"),
                                            ("base", "andnq %rsi, %rdi, %rax", "bmi1"),
                                            ("avx2", "andnq %rsi, %rdi, %rax", "bmi1"),
                                            ("avx2", "pextq %rsi, %rdi, %rax", "bmi2")]:
            errors = self.errors(universal_image(plant=(region, instruction)))
            self.assertTrue(any(e.startswith(f"{region}:") and family in e for e in errors), (region, family, errors))

    def test_the_pext_tier_may_use_every_family(self):
        self.assertEqual(self.errors(universal_image(plant=("pext", "andnq %rsi, %rdi, %rax"))), [])

    def test_a_hot_function_off_its_line_fails(self):
        errors = self.errors(universal_image(misalign=("pext", "N8Searcher7negamax")))
        self.assertTrue(any("hot function" in e and "negamax" in e and "16 mod 64" in e for e in errors), errors)

    def test_a_missing_hot_function_fails(self):
        text = "\n".join(line for line in universal_image().splitlines() if "Board6see_ge" not in line or "avx2" not in line)
        self.assertTrue(any("avx2: no function matching" in e for e in self.errors(text)))

    def test_fathom_in_a_tier_fails(self):
        errors = self.errors(universal_image(extra_fathom="tb_init"))
        self.assertTrue(any("Fathom's tb_init" in e for e in errors), errors)


class RuntimeGatedTests(unittest.TestCase):
    TEXT = """
0000000140001000 <_ZNK9libunwind15Registers_arm6411getRegisterEi>:
   140001000: 04e0e3e0     	cntd	x0
   140001004: d65f03c0     	ret
0000000140001010 <Searcher::negamax>:
   140001010: f8a86800     	prfm	pldl1keep, [x0, x8]
"""

    def test_a_gated_helper_is_allowed_and_named(self):
        counts, allowed, offending = verify_isa.arm64_split(self.TEXT)
        self.assertEqual(offending, [])
        self.assertEqual(len(allowed), 1)
        self.assertIn("<_ZNK9libunwind15Registers_arm6411getRegisterEi>", allowed[0])
        _, errors = verify_isa.verify_binary("arm64", counts)
        self.assertEqual(errors, [])

    def test_the_same_instruction_elsewhere_fails_and_is_named(self):
        text = self.TEXT.replace("<Searcher::negamax>:",
                                 "<Searcher::negamax>:\n   14000100c: 04e0e3e0     \tcntd\tx0")
        counts, _, offending = verify_isa.arm64_split(text)
        self.assertEqual(len(offending), 1)
        self.assertIn("<Searcher::negamax>", offending[0])
        _, errors = verify_isa.verify_binary("arm64", counts)
        self.assertTrue(any("forbidden sve" in e for e in errors))


class RuntimeArchiveTests(unittest.TestCase):
    TEXT = """
0000000000401000 <__strlen_evex>:
  401000: f3 48 0f b8 c7               popcntq %rdi, %rax
  401005: c3                           retq
0000000000401010 <Searcher::negamax>:
  401010: c3                           retq
"""

    def test_a_runtime_archive_function_is_left_out(self):
        counts = verify_isa.instruction_counts(self.TEXT, frozenset({"__strlen_evex"}))
        self.assertEqual(counts["popcntq"], 0)
        _, errors = verify_isa.verify_binary("portable", counts)
        self.assertEqual(errors, [])

    def test_the_same_instruction_in_engine_code_still_fails(self):
        text = self.TEXT.replace("  401010: c3",
                                 "  40100c: f3 48 0f b8 c7               popcntq %rdi, %rax\n  401010: c3")
        counts = verify_isa.instruction_counts(text, frozenset({"__strlen_evex"}))
        self.assertEqual(counts["popcntq"], 1)
        _, errors = verify_isa.verify_binary("portable", counts)
        self.assertTrue(any("forbidden popcnt" in e for e in errors))

    def test_offending_functions_are_named(self):
        self.assertEqual(verify_isa.x86_offenders(self.TEXT), {"__strlen_evex": 1})
        self.assertEqual(verify_isa.x86_offenders(self.TEXT, frozenset({"__strlen_evex"})), {})

    def test_nothing_is_left_out_without_an_archive(self):
        _, errors = verify_isa.verify_binary("portable", verify_isa.instruction_counts(self.TEXT))
        self.assertTrue(any("forbidden popcnt" in e for e in errors))


if __name__ == "__main__":
    unittest.main()
