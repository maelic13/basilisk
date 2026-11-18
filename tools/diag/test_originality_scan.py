from __future__ import annotations

import importlib.util
import json
import pathlib
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "originality_scan", ROOT / "tools" / "diag" / "originality_scan.py"
)
scan = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(scan)

DONOR_FUNC = """
int donor_search(Position& pos, int depth, int alpha, int beta) {
    int best = -32001;
    for (int i = 0; i < pos.count(); ++i) {
        Move m = pos.moves[i];
        pos.do_move(m);
        int v = -donor_search(pos, depth - 1, -beta, -alpha);
        pos.undo_move(m);
        if (v > best) best = v;
        if (best >= beta) break;
        if (best > alpha) alpha = best;
    }
    return best;
}
"""
DONOR_TABLE = "const int PawnPst[8] = {0, 98, 134, 61, 95, 68, 126, 34};\n"

# Same tokens as DONOR_FUNC, but a different layout and a comment.
PLANTED_FUNC = DONOR_FUNC.replace("    ", "  ").replace("{\n", "{ // moved\n", 1)

UNRELATED = """
// Unrelated engine code.
static uint64_t mix(uint64_t x) {
    x ^= x >> 33;
    x *= 0xff51afd7ed558ccdULL;
    return x;
}
struct Clock { long start; long elapsed() const { return now() - start; } };
"""


def make_tree(root: pathlib.Path, files: dict[str, str]) -> str:
    for name, text in files.items():
        p = root / "src" / name
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(text.encode("utf-8"))
    return str(root)


class StripTests(unittest.TestCase):
    def test_comments_strings_and_chars_blanked_keeping_lines(self):
        src = 'int a = 1; // 99 98 97\n/* 5\n6 */ int b = 2; const char* s = "7, 8";\nchar c = \'9\';\n'
        out = scan.strip(src)
        self.assertEqual(out.count("\n"), src.count("\n"))
        self.assertEqual(len(out), len(src))
        nums = [scan.parse_num(m.group(1)) for m in scan.NUM_RE.finditer(out)]
        self.assertEqual(nums, [1, 2])

    def test_digit_separator_is_not_a_char_literal(self):
        out = scan.strip("int x = 1'000'000;")
        self.assertEqual([scan.parse_num(m.group(1)) for m in scan.NUM_RE.finditer(out)], [1000000])


class NumberTests(unittest.TestCase):
    def test_cpp_literal_forms(self):
        text = "a = 0xFFu; b = 0b101; c = 12ULL; d = 1.5f; e = -7; f = x2 + 3;"
        vals = [scan.parse_num(m.group(1)) for m in scan.NUM_RE.finditer(text)]
        self.assertEqual(vals, [255, 5, 12, 1.5, -7, 3])

    def test_trivial_threshold_logic(self):
        self.assertTrue(scan.trivial([0, 1, -1, 2, 0]))
        self.assertTrue(scan.trivial([1, 2, 3, 4, 5]))
        self.assertTrue(scan.trivial([7, 7, 7, 7]))
        self.assertTrue(scan.trivial([1, 2, 4, 8, 16]))
        self.assertFalse(scan.trivial([98, 134, 61, 95, 68]))

    def test_common_runs_min_length(self):
        a = [(v, 1) for v in [9, 10, 11, 50, 60, 70, 80, 90, 3]]
        b = [(v, 1) for v in [1, 50, 60, 70, 80, 90, 2]]
        self.assertEqual(scan.common_runs(a, b, 5), [(3, 1, 5)])
        self.assertEqual(scan.common_runs(a, b, 6), [])

    def test_scaled_match_needs_nonunit_k_and_variety(self):
        base = [10, 40, 25, 70, 5, 90]
        donor = {"d.cpp": [(v, 1) for v in base]}
        scaled = [2 * v for v in base]
        self.assertEqual(len(scan.scaled_matches([("c.cpp", 1, scaled)], donor)), 1)
        self.assertEqual(scan.scaled_matches([("c.cpp", 1, list(base))], donor), [])
        self.assertEqual(scan.scaled_matches([("c.cpp", 1, [5, 5, 5, 5, 5, 5])], donor), [])


class TokenTests(unittest.TestCase):
    def test_run_threshold(self):
        cand = {"c.cpp": [(t, 1) for t in "a b c d e f g h".split()]}
        donor = {"d.cpp": [(t, 1) for t in "x a b c d e f y".split()]}
        self.assertEqual(scan.token_runs(cand, donor, 6)[0]["tokens"], 6)
        self.assertEqual(scan.token_runs(cand, donor, 7), [])

    def test_normalised_tokens_ignore_identifier_names(self):
        with tempfile.TemporaryDirectory() as d:
            t1 = make_tree(pathlib.Path(d) / "a", {"x.cpp": "int foo = bar + 1;"})
            raw = scan.tokens(t1, False)["src/x.cpp"]
            norm = scan.tokens(t1, True)["src/x.cpp"]
        self.assertEqual([t for t, _ in raw], ["int", "foo", "=", "bar", "+", "1", ";"])
        self.assertEqual([t for t, _ in norm], ["ID", "ID", "=", "ID", "+", "1", ";"])


class EndToEndTests(unittest.TestCase):
    def run_scan(self, cand_files, donor_files, exclude=()):
        with tempfile.TemporaryDirectory() as d:
            d = pathlib.Path(d)
            cand = make_tree(d / "cand", cand_files)
            donor = make_tree(d / "donor", donor_files)
            out = d / "out"
            scan.run(str(out), cand, {"don": donor}, exclude)
            rec = json.loads((out / "don.json").read_text())
            summary = json.loads((out / "summary.json").read_text())
        return rec, summary

    def donor_files(self):
        return {"search.cpp": DONOR_FUNC, "pst.h": DONOR_TABLE, "util.cpp": UNRELATED}

    def test_positive_control_function_and_table(self):
        cand = {"mine.cpp": "// mine\n" + PLANTED_FUNC,
                "tables.h": "const int Pst[8] = {0, 98, 134, 61, 95, 68, 126, 34};\n"}
        rec, summary = self.run_scan(cand, self.donor_files())
        self.assertTrue(any(r["cand"] == "src/mine.cpp" and r["tokens"] >= scan.TOKEN_RAW_K
                            for r in rec["token_runs_raw_k20"]))
        self.assertTrue(any(r["tokens"] >= scan.TOKEN_NORM_K for r in rec["token_runs_norm_k40"]))
        nt = rec["numeric_runs_nontrivial"]
        self.assertTrue(any(r["cand"] == "src/tables.h" and r["donor"] == "src/pst.h" and r["len"] >= 7
                            for r in nt))
        self.assertEqual(summary["donors"]["don"]["token_runs_raw_k20"], len(rec["token_runs_raw_k20"]))

    def test_positive_control_scaled_table(self):
        cand = {"tables.h": "const int Pst[8] = {0, 196, 268, 122, 190, 136, 252, 68};\n"}
        rec, _ = self.run_scan(cand, self.donor_files())
        self.assertTrue(any(h["cand"] == "src/tables.h" and h["k"] == 2.0 for h in rec["scaled"]))

    def test_negative_control(self):
        cand = {"other.cpp": """
            #include <vector>
            double ratio(double a, double b) { return a / (b + 3.14159); }
            class Timer { public: void tick() { ++n_; } private: int n_ = 0; };
            const int Weights[5] = {3, 17, 29, 41, 53};
            """}
        rec, _ = self.run_scan(cand, self.donor_files())
        self.assertEqual(rec["token_runs_raw_k20"], [])
        self.assertEqual(rec["token_runs_norm_k40"], [])
        self.assertEqual(rec["scaled"], [])
        self.assertEqual(rec["numeric_runs_nontrivial"], [])

    def test_exclude_skips_subdirectory(self):
        cand = {"vendored/copy.cpp": PLANTED_FUNC}
        rec, _ = self.run_scan(cand, self.donor_files())
        self.assertTrue(rec["token_runs_raw_k20"])
        rec, _ = self.run_scan(cand, self.donor_files(), exclude=["src/vendored"])
        self.assertEqual(rec["token_runs_raw_k20"], [])

    def test_comments_do_not_create_matches(self):
        cand = {"c.cpp": "// " + DONOR_FUNC.replace("\n", "\n// ") + "\nint z;\n"}
        rec, _ = self.run_scan(cand, self.donor_files())
        self.assertEqual(rec["token_runs_raw_k20"], [])


class CliTests(unittest.TestCase):
    def test_main_writes_outputs(self):
        with tempfile.TemporaryDirectory() as d:
            d = pathlib.Path(d)
            cand = make_tree(d / "c", {"a.cpp": "int x = 1;"})
            donor = make_tree(d / "d", {"a.cpp": "int y = 2;"})
            self.assertEqual(scan.main([str(d / "o"), cand, "dn=" + donor]), 0)
            self.assertTrue((d / "o" / "dn.json").exists())
            self.assertTrue((d / "o" / "summary.json").exists())


if __name__ == "__main__":
    unittest.main()
