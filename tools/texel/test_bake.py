import unittest
from pathlib import Path

import bake

HEADER = Path(__file__).resolve().parents[2] / "src" / "eval_params.h"


class BakeProvenanceTests(unittest.TestCase):
    """A dump bakes only if it is a whole tuner vector fitted from this header."""

    def setUp(self):
        self.header = HEADER.read_text(encoding="utf-8")
        self.params = bake.parse_param_list(self.header)
        self.vector = bake.header_vector(self.header, self.params)
        self.root = bake.vector_hash(self.vector)

    def dump(self, vector, root=None):
        lines = [f"# {bake.VECTOR_SCHEMA}", f"# root {root or self.root}", f"# seed {self.root}"]
        lines += [f"{name} {index} {value}" for name, index, value in vector]
        return "\n".join(lines) + "\n"

    def errors(self, text):
        return bake.check_dump(text, self.params, self.header)

    def test_a_whole_vector_from_this_header_passes(self):
        self.assertEqual(len(self.vector), 1159)
        self.assertEqual(self.errors(self.dump(self.vector)), [])

    def test_a_missing_member_is_refused(self):
        errors = self.errors(self.dump([v for v in self.vector if v[0] != "Tempo"]))
        self.assertTrue(any("1 slots missing" in e and "Tempo[0]" in e for e in errors), errors)

    def test_an_unknown_name_is_refused(self):
        errors = self.errors(self.dump(self.vector + [("NoSuchTerm", 0, 1)]))
        self.assertTrue(any("unknown name NoSuchTerm" in e for e in errors), errors)

    def test_a_stale_dump_is_refused(self):
        errors = self.errors(self.dump(self.vector, root="0123456789abcdef"))
        self.assertTrue(any("stale" in e for e in errors), errors)

    def test_a_dump_without_provenance_is_refused(self):
        bare = "\n".join(f"{n} {i} {v}" for n, i, v in self.vector) + "\n"
        self.assertTrue(any("schema" in e for e in self.errors(bare)))


class BakeFormattingTests(unittest.TestCase):
    def test_2d_span_includes_existing_indentation(self):
        header = (
            "struct P {\n"
            "        int table[2][2] = {\n"
            "            { 1, 2 },\n"
            "            { 3, 4 }\n"
            "        };\n"
            "};\n"
        )

        span, rows, cols, values = bake.find_2d_span(header, "table")

        self.assertEqual((rows, cols), (2, 2))
        self.assertEqual(values, [[1, 2], [3, 4]])
        self.assertEqual(header[span[0]:span[0] + 8], "        ")

    def test_rewrite_does_not_accumulate_indentation(self):
        header = "struct P {\n    int table[1][2] = {\n        { 1, 2 }\n    };\n};\n"
        span, rows, cols, _ = bake.find_2d_span(header, "table")
        replacement = bake.format_2d_block("table", rows, cols, [[2, 3]], [])
        rewritten = header[:span[0]] + replacement + header[span[1]:]

        self.assertIn("\n    int table[1][2] = {", rewritten)
        self.assertNotIn("\n        int table[1][2] = {", rewritten)


if __name__ == "__main__":
    unittest.main()
