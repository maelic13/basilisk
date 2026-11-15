"""Tests for hot_path_calls.py on synthetic disassembly: no binary is needed."""
from __future__ import annotations

import importlib.util
import pathlib
import re
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("hot_path_calls", ROOT / "tools" / "diag" / "hot_path_calls.py")
hpc = importlib.util.module_from_spec(SPEC)
assert SPEC.loader
SPEC.loader.exec_module(hpc)

DISASSEMBLY = """
0000000140001000 <int Searcher::negamax<(NodeType)2>(int, int, int, int, SearchStack*, bool)>:
140001010:     	callq	0x140002000 <Board::make_move(unsigned short)>
140001020:     	jmp	0x140001010 <int Searcher::negamax<(NodeType)2>(int, int, int, int, SearchStack*, bool)+0x10>
140001030:     	callq	*%rax
140001040:     	callq	0x140005000 <std::__1::__throw_length_error[abi:x](char const*)>
140001050:     	callq	0x140006000 <helper()>
0000000140002000 <Board::make_move(unsigned short)>:
140002010:     	callq	0x140003000 <operator new(unsigned long long)>
0000000140005000 <std::__1::__throw_length_error[abi:x](char const*)>:
140005010:     	callq	0x140003000 <operator new(unsigned long long)>
0000000140006000 <helper()>:
140006010:     	callq	*0x10(%rip)              # 0x140009000 <__imp_malloc>
140006020:     	callq	0x140003000 <operator new(unsigned long long)>
0000000140003000 <operator new(unsigned long long)>:
140003010:     	callq	*0x10(%rip)              # 0x140009000 <__imp_malloc>
"""

NEGAMAX = "int Searcher::negamax<(NodeType)2>(int, int, int, int, SearchStack*, bool)"


class HotPathTests(unittest.TestCase):
    def setUp(self):
        self.edges, self.sites, self.indirect = hpc.parse(DISASSEMBLY)

    def walk(self):
        return hpc.walk(self.edges, [NEGAMAX], [re.compile(p) for p in hpc.FORBIDDEN],
                        [re.compile(p) for p in hpc.TERMINAL])

    def test_parse_reads_calls_tail_jumps_imports_and_indirect_calls(self):
        self.assertEqual(self.edges[NEGAMAX],
                         {"Board::make_move(unsigned short)",
                          "std::__1::__throw_length_error[abi:x](char const*)", "helper()"})
        self.assertEqual(self.indirect[NEGAMAX], 1)
        self.assertIn("__imp_malloc", self.edges["helper()"])
        self.assertEqual(self.sites[(NEGAMAX, "Board::make_move(unsigned short)")], ["0x140001010"])

    def test_every_caller_of_an_allocator_is_its_own_hit_and_errors_are_not_followed(self):
        hits, reached = self.walk()
        pairs = sorted((path[-2], path[-1]) for path in hits)
        self.assertEqual(pairs, [("Board::make_move(unsigned short)", "operator new(unsigned long long)"),
                                 ("helper()", "__imp_malloc"),
                                 ("helper()", "operator new(unsigned long long)")])
        # The throwing path is reached but not descended.
        self.assertIn("std::__1::__throw_length_error[abi:x](char const*)", reached)

    def test_a_known_path_does_not_cover_another_caller_of_the_same_allocator(self):
        hits, _ = self.walk()
        known = [re.compile(r) for r, _ in hpc.KNOWN]
        unknown = []
        for path in hits:
            text = hpc.render(path, self.sites, {})
            if not any(p.search(text) for p in known):
                unknown.append(path[-2])
        # make_move's allocation names no UndoInfo here, so nothing is known.
        self.assertEqual(sorted(unknown), ["Board::make_move(unsigned short)", "helper()", "helper()"])

    def test_inline_chains_classify_an_inlined_frame(self):
        path = [NEGAMAX, "operator new(unsigned long long)"]
        sites = {(NEGAMAX, "operator new(unsigned long long)"): ["0x1"]}
        chains = {"0x1": ["__libcpp_allocate<UndoInfo>", "push_back", "make_move", NEGAMAX]}
        text = hpc.render(path, sites, chains)
        self.assertIn("[__libcpp_allocate<UndoInfo> <- push_back <- make_move]", text)
        self.assertTrue(re.search(hpc.KNOWN[0][0], text))
        # Without the chain the same site is not known.
        self.assertFalse(re.search(hpc.KNOWN[0][0], hpc.render(path, sites, {})))


if __name__ == "__main__":
    unittest.main()
