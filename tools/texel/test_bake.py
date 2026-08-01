import unittest

import bake


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
