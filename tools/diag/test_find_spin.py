#!/usr/bin/env python3
"""find_spin.py counts what does not answer, and only that.

The engines here are small scripts with a known behaviour: a read that passes an
engine which never answers would pass anything, so that is the case that matters.
"""

import pathlib
import subprocess
import sys
import tempfile
import textwrap
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import find_spin

FEN = "2rr3k/pp3pp1/1nnqbN1p/3pN3/2pP4/2P3Q1/PPB4P/R4RK1 w - - 0 1"
READ_ALL = "import sys\nfor line in sys.stdin:\n"
ANSWERS = READ_ALL + "    if line.startswith('go'):\n        print('bestmove e2e4', flush=True)\n"
NEVER = READ_ALL + "    if line.startswith('go'):\n        print('info depth 1 score cp 0', flush=True)\n"
DIES = "import sys\nsys.stdin.readline()\nsys.exit(3)\n"
NAMES_OPTIONS = READ_ALL + textwrap.indent(textwrap.dedent("""\
    if line.startswith('setoption name Wanted value 7'):
        seen = True
    if line.startswith('go') and 'seen' in globals():
        print('bestmove e2e4', flush=True)
"""), "    ")


class FindSpinTest(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.directory = pathlib.Path(directory.name)

    def finishes(self, source, limit=1.0, options=()):
        script = self.directory / "engine.py"
        script.write_text(source, encoding="utf-8")
        return find_spin.search_finishes([sys.executable, str(script)], FEN, 6, limit, list(options), 16)[0]

    def test_an_engine_that_answers_finishes(self):
        self.assertTrue(self.finishes(ANSWERS, limit=20.0))

    def test_an_engine_that_never_answers_is_counted(self):
        self.assertFalse(self.finishes(NEVER))

    def test_an_engine_that_dies_is_counted(self):
        self.assertFalse(self.finishes(DIES, limit=20.0))

    def test_an_option_reaches_the_engine(self):
        self.assertTrue(self.finishes(NAMES_OPTIONS, limit=20.0, options=["Wanted=7"]))
        self.assertFalse(self.finishes(NAMES_OPTIONS, options=["Wanted=8"]))

    def test_the_tool_exits_one_on_an_unfinished_position(self):
        # The whole tool, its exit status read directly, on an engine that is a real
        # executable: this interpreter, which waits on stdin and never says bestmove.
        epd = self.directory / "one.epd"
        epd.write_text(FEN.rsplit(" ", 2)[0] + ' bm Qg6; id "X.1";\n', encoding="utf-8")
        tool = pathlib.Path(find_spin.__file__)
        code = subprocess.run([sys.executable, str(tool), "--engine", sys.executable, "--epd", str(epd),
                               "--limit", "1"], capture_output=True, text=True, timeout=60).returncode
        self.assertEqual(code, 1)

    def test_the_tool_refuses_a_missing_engine(self):
        tool = pathlib.Path(find_spin.__file__)
        code = subprocess.run([sys.executable, str(tool), "--engine", str(self.directory / "none.exe")],
                              capture_output=True, text=True, timeout=60).returncode
        self.assertEqual(code, 2)

    def test_the_positions_are_read_from_their_four_fields(self):
        epd = self.directory / "two.epd"
        epd.write_text('8/8/8/8/8/8/8/K1k5 w - - bm Kb1; id "X.1";\n\n8/8/8/8/8/8/8/K1k5 b - - 3 9\n', encoding="utf-8")
        self.assertEqual(find_spin.read_positions(epd), ["8/8/8/8/8/8/8/K1k5 w - - 0 1", "8/8/8/8/8/8/8/K1k5 b - - 0 1"])


if __name__ == "__main__":
    unittest.main()
