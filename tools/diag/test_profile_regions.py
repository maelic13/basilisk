import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import profile_regions

REPO = pathlib.Path(__file__).resolve().parents[2]
SRC = "D:/code/basilisk/src/"
LIBCXX = "C:/msys64/clang64/include/c++/v1/__algorithm/min.h"
BASE = 0x140000000

# Innermost frame first, as llvm-symbolizer prints them.
CHAINS = {
    0x1000: [("Board::make_move(Move)", SRC + "board.cpp"), ("negamax(...)", SRC + "search_kernel_core.cpp")],
    0x2000: [("std::min<int>(int const&, int const&)", LIBCXX), ("evaluate(Board const&)", SRC + "eval.cpp"),
             ("negamax(...)", SRC + "search_kernel_core.cpp")],
    0x3000: [("negamax(...)", SRC + "search_kernel_core.cpp")],
    0x4000: [("memcpy", "??")],
    0x6000: [("??", "??")],
}


def samples(counts, outside=0):
    return {"schema": profile_regions.SAMPLES_SCHEMA, "image_base": hex(BASE), "label": "t", "cohort": "c",
            "in_module": {hex(rva): n for rva, n in counts.items()}, "outside_module": outside}


def chains(table=CHAINS):
    return {BASE + rva: frames for rva, frames in table.items()}


class ParseTests(unittest.TestCase):
    def test_windows_paths_and_inline_chains(self):
        text = ("0x140001000\nBoard::make_move(Move)\nD:\\code\\basilisk\\src\\board.cpp:120:7\n"
                "negamax(...)\nD:\\code\\basilisk\\src\\search_kernel_core.cpp:880:13\n\n"
                "0x140004000\nmemcpy\n??:0:0\n")
        parsed = profile_regions.parse_symbolizer(text)
        self.assertEqual(parsed[0x140001000], [("Board::make_move(Move)", "D:\\code\\basilisk\\src\\board.cpp"),
                                               ("negamax(...)", "D:\\code\\basilisk\\src\\search_kernel_core.cpp")])
        self.assertEqual(parsed[0x140004000], [("memcpy", "??")])


class AttributionTests(unittest.TestCase):
    def setUp(self):
        self.map = profile_regions.load_map()

    def test_innermost_engine_frame_decides(self):
        report = profile_regions.analyse(samples({0x1000: 50, 0x2000: 30, 0x3000: 19, 0x4000: 1}, outside=0),
                                         chains(), self.map)
        regions = report["regions"]
        self.assertAlmostEqual(regions["board"]["share"], 0.50)
        self.assertAlmostEqual(regions["eval"]["share"], 0.30)  # std::min is skipped outward
        self.assertAlmostEqual(regions["search"]["share"], 0.19)
        self.assertAlmostEqual(regions["runtime"]["share"], 0.01)
        self.assertAlmostEqual(regions["search"]["inline_inclusive"], 0.99)
        self.assertAlmostEqual(report["inline_share"], 0.80)

    def test_outside_samples_are_system(self):
        report = profile_regions.analyse(samples({0x1000: 90}, outside=10), chains(), self.map)
        self.assertAlmostEqual(report["regions"]["system"]["share"], 0.10)

    def test_no_inline_chains_is_refused(self):
        flat = {rva: frames[-1:] for rva, frames in CHAINS.items()}
        with self.assertRaisesRegex(profile_regions.ProfileRefused, "inline chain"):
            profile_regions.analyse(samples({0x1000: 50, 0x2000: 30, 0x3000: 20}), chains(flat), self.map)

    def test_unresolved_samples_are_refused(self):
        with self.assertRaisesRegex(profile_regions.ProfileRefused, "no function name"):
            profile_regions.analyse(samples({0x1000: 90, 0x6000: 10}), chains(), self.map)

    def test_samples_without_engine_files_are_refused(self):
        with self.assertRaisesRegex(profile_regions.ProfileRefused, "no line tables"):
            profile_regions.analyse(samples({0x1000: 80, 0x4000: 20}), chains(), self.map)

    def test_an_unmapped_engine_file_is_refused(self):
        table = dict(CHAINS)
        table[0x5000] = [("newthing()", SRC + "new_module.cpp"), ("negamax(...)", SRC + "search_kernel_core.cpp")]
        with self.assertRaisesRegex(profile_regions.ProfileRefused, "new_module.cpp"):
            profile_regions.analyse(samples({0x1000: 90, 0x5000: 10}), chains(table), self.map)

    def test_a_missing_address_is_refused(self):
        with self.assertRaisesRegex(profile_regions.ProfileRefused, "returned nothing"):
            profile_regions.analyse(samples({0x9000: 10}), chains(), self.map)


class MapTests(unittest.TestCase):
    def test_every_engine_source_file_is_mapped(self):
        mapped = profile_regions.load_map()["by_file"]
        files = [p for p in (REPO / "src").iterdir() if p.suffix in (".cpp", ".h")]
        files += [p for p in (REPO / "external" / "fathom" / "src").iterdir() if p.suffix in (".c", ".h")]
        missing = sorted(p.name for p in files if p.name not in mapped)
        self.assertEqual(missing, [], "add these files to tools/diag/profile_regions.json")

    def test_a_file_maps_to_one_region(self):
        regions = profile_regions.load_map()["regions"]
        names = [name for _, files in regions for name in files]
        self.assertEqual(len(names), len(set(names)))


if __name__ == "__main__":
    unittest.main()
