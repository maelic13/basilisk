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


if __name__ == "__main__":
    unittest.main()
