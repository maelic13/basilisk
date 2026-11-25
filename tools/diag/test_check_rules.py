import pathlib
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import check_rules
from check_rules import CheckError

HEADER = ("Scope: engine code. Read in full when: Task -- any `I1` leaf. "
          "Paths -- `src/`, `CMakeLists.txt`. Actions -- before writing code.")
CORE = ("# Contract\n\n## Rules by task\n\n"
        "- **`agents/code.md`** -- Scope: engine code. Read in full when: Task --\n"
        "  any `I1` leaf. Paths -- `src/`, `CMakeLists.txt`. Actions -- before\n"
        "  writing code.\n\n"
        "## Changing these rules\n\n- Budget: **2,000** bytes for this file.\n")
MODULE = ("# Code\n\nScope: engine code.\n\n"
          "Read in full when: Task -- any `I1` leaf. Paths -- `src/`,\n"
          "`CMakeLists.txt`. Actions -- before writing code.\n\n"
          "A pointer that is not part of the header.\n\n## Rules\n\n- A rule.\n")
STUB = "# Stub\n\n@AGENTS.md\n\n- A tool-specific rule, with `@agents/code.md` only in a code span.\n"


def write_tree(root, core=CORE, module=MODULE, stub=STUB):
    root = pathlib.Path(root)
    (root / "agents").mkdir(exist_ok=True)
    (root / "AGENTS.md").write_text(core, encoding="utf-8", newline="\n")
    (root / "agents" / "code.md").write_text(module, encoding="utf-8", newline="\n")
    (root / "CLAUDE.md").write_text(stub, encoding="utf-8", newline="\n")
    return root


class CheckRulesTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = write_tree(self.temp.name)

    def tearDown(self):
        self.temp.cleanup()

    def assert_fails(self, fragment):
        with self.assertRaises(CheckError) as caught:
            check_rules.check(self.root)
        self.assertIn(fragment, str(caught.exception))

    def test_a_valid_tree_passes_and_reports_each_set(self):
        report = "\n".join(check_rules.check(self.root))
        self.assertIn("loaded by the core alone: AGENTS.md", report)
        self.assertIn("loaded by Claude Code: CLAUDE.md + AGENTS.md", report)
        self.assertIn("module agents/code.md", report)

    def test_a_size_counts_the_crlf_form_whatever_the_checkout(self):
        lf = check_rules.loaded_bytes(self.root / "AGENTS.md")
        (self.root / "AGENTS.md").write_bytes(CORE.replace("\n", "\r\n").encode("utf-8"))
        self.assertEqual(check_rules.loaded_bytes(self.root / "AGENTS.md"), lf)
        self.assertEqual(lf, len(CORE.encode("utf-8")) + CORE.count("\n"))

    def test_a_core_over_budget_fails(self):
        write_tree(self.root, core=CORE + "- A rule too many.\n" * 100)
        self.assert_fails("loads")
        self.assert_fails("over the budget of 2,000")

    def test_a_stub_counts_toward_the_set_it_loads(self):
        # The core alone fits; the stub pushes what Claude Code loads over.
        write_tree(self.root, stub=STUB + "- A stub rule.\n" * 110)
        with self.assertRaises(CheckError) as caught:
            check_rules.check(self.root)
        self.assertIn("Claude Code loads", str(caught.exception))
        self.assertNotIn("the core alone loads", str(caught.exception))

    def test_a_module_over_budget_fails(self):
        write_tree(self.root, module=MODULE + "- A rule too many.\n" * 120)
        self.assert_fails("agents/code.md: ")
        self.assert_fails("exceeds the module budget")

    def test_a_module_outside_the_index_fails(self):
        (self.root / "agents" / "extra.md").write_text(MODULE, encoding="utf-8")
        self.assert_fails("agents/extra.md: a module the index in AGENTS.md does not name")

    def test_an_index_entry_without_its_module_fails(self):
        (self.root / "agents" / "code.md").unlink()
        self.assert_fails("the index names agents/code.md, which does not exist")

    def test_a_stub_importing_a_module_fails(self):
        write_tree(self.root, stub=STUB + "\n@agents/code.md\n")
        self.assert_fails("Claude Code loads the module agents/code.md on every task")

    def test_an_antigravity_include_of_a_module_fails(self):
        (self.root / "GEMINI.md").write_text("@[code](agents/code.md)\n", encoding="utf-8")
        self.assert_fails("Gemini CLI and Antigravity loads the module agents/code.md")

    def test_an_import_inside_a_code_span_or_fence_is_not_an_import(self):
        write_tree(self.root, stub=STUB + "\n```\n@agents/code.md\n```\n")
        check_rules.check(self.root)

    def test_a_module_without_its_header_fails(self):
        write_tree(self.root, module="# Code\n\nRead this for code.\n\n## Rules\n\n- A rule.\n")
        self.assert_fails("agents/code.md: does not open with its scope and triggers")

    def test_a_header_that_differs_from_the_index_fails(self):
        write_tree(self.root, module=MODULE.replace("before writing code.", "before any code."))
        self.assert_fails("agents/code.md: does not open with its scope and triggers")

    def test_a_trigger_added_to_the_module_alone_fails(self):
        write_tree(self.root, module=MODULE.replace("writing code.\n\nA pointer",
                                                    "writing code. And tests.\n\nA pointer"))
        self.assert_fails("agents/code.md: does not open with its scope and triggers")

    def test_an_index_entry_without_a_trigger_kind_fails(self):
        write_tree(self.root,
                   core=CORE.replace(" Paths -- `src/`, `CMakeLists.txt`.", ""),
                   module=MODULE.replace(" Paths -- `src/`,\n`CMakeLists.txt`.", ""))
        self.assert_fails("the index entry for agents/code.md lacks Paths --")

    def test_a_missing_or_doubled_budget_fails(self):
        write_tree(self.root, core=CORE.replace("Budget: **2,000** bytes", "No budget"))
        self.assert_fails("expected one 'Budget: **<n>** bytes' declaration, found 0")
        write_tree(self.root, core=CORE + "- Budget: **9,000** bytes as well.\n")
        self.assert_fails("found 2")

    def test_a_nested_rule_file_fails_unless_it_is_reference_data(self):
        nested = self.root / "docs" / "reference" / "donor"
        nested.mkdir(parents=True)
        (nested / "AGENTS.md").write_text("Another repository's rules.\n", encoding="utf-8")
        self.assertIn("1 rule files under docs/reference/ are reference data",
                      "\n".join(check_rules.check(self.root)))
        (self.root / "src").mkdir()
        (self.root / "src" / "CLAUDE.md").write_text("Loaded on demand.\n", encoding="utf-8")
        self.assert_fails("src/CLAUDE.md: a rule file a tool may load")

    def test_path_triggers_fire_on_a_file_or_under_a_directory(self):
        entry = check_rules.index_entries(CORE)["agents/code.md"]
        self.assertEqual(entry, HEADER)
        triggers = check_rules.path_triggers(entry)
        self.assertEqual(triggers, ["`src/`", "`CMakeLists.txt`"])
        changed = ["CMakeLists.txt", "docs/PLAN.md", "src/search.cpp", "tests/src/x.cpp"]
        self.assertEqual(check_rules.fired(triggers, changed), ["CMakeLists.txt", "src/search.cpp"])
        self.assertEqual(
            check_rules.fired(["`tools/run_*`"], ["tools/run_x.py", "tools/sprt.ps1", "run_y.py"]),
            ["tools/run_x.py"])


if __name__ == "__main__":
    unittest.main()
