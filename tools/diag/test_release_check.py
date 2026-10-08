import pathlib
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import release_check
from release_check import CheckError

GUIDE = ("| Item | Value |\n|---|---|\n"
         "| Released baseline | **1.10.2** on `master`, tagged `v1.10.2` (2026-10-20) |\n"
         "| Bench fingerprint | **14,978,465** (`bench 13`) |\n")
CHANGELOG = ("# Changelog\n\n## [Unreleased]\n\n---\n\n## [1.10.2] - 2026-10-20\n\n"
             "A patch.\n\n### Fixed\n\n- A thing.\n\n---\n\n## [1.10.1] - 2026-09-27\n\nOlder.\n")


def write_tree(root, engine='"1.10.2"', cmake="1.10.2", guide=GUIDE, changelog=CHANGELOG):
    root = pathlib.Path(root)
    (root / "src").mkdir(exist_ok=True)
    (root / "docs").mkdir(exist_ok=True)
    (root / "src" / "constants.h").write_text(
        f"inline constexpr std::string_view engineVersion = {engine};\n", encoding="utf-8")
    (root / "CMakeLists.txt").write_text(
        f"cmake_minimum_required(VERSION 3.24)\nproject(basilisk VERSION {cmake} LANGUAGES CXX)\n",
        encoding="utf-8")
    (root / "GUIDE.md").write_text(guide, encoding="utf-8")
    (root / "CHANGELOG.md").write_text(changelog, encoding="utf-8")
    (root / "docs" / "DESIGN.md").write_text("- **Bench signature**: **14,978,465**\n", encoding="utf-8")
    (root / "AGENTS.md").write_text("(currently **14,978,465** at `bench 13`)\n", encoding="utf-8")
    for command in (["git", "init", "-q"], ["git", "add", "-A"],
                    ["git", "-c", "user.name=t", "-c", "user.email=t@t", "commit", "-q", "-m", "x"]):
        subprocess.run(command, cwd=root, check=True)
    return root


class ReleaseCheckTests(unittest.TestCase):
    def test_a_release_commit_passes_and_writes_its_notes(self):
        with tempfile.TemporaryDirectory() as temp:
            root = write_tree(temp)
            notes = root / "notes.md"
            message = release_check.check(root, "v1.10.2", "HEAD", notes)
            self.assertIn("bench 13 declared 14978465", message)
            self.assertEqual(notes.read_text(encoding="utf-8"), "A patch.\n\n### Fixed\n\n- A thing.\n")

    def test_each_defect_is_refused(self):
        cases = {
            "tag format": (dict(), "1.10.2"),
            "tag names another version": (dict(), "v1.10.3"),
            "sources disagree": (dict(cmake="1.10.1"), "v1.10.2"),
            "a -dev version is not a release": (dict(engine='"1.10.2-dev"'), "v1.10.2"),
            "undated section": (dict(changelog=CHANGELOG.replace("] - 2026-10-20", "]")), "v1.10.2"),
            "empty section": (dict(changelog="## [1.10.2] - 2026-10-20\n\n---\n"), "v1.10.2"),
            "GUIDE not marked": (dict(guide=GUIDE.replace("**1.10.2**", "**1.10.1**")), "v1.10.2"),
            "fingerprint restated stale": (dict(guide=GUIDE.replace("14,978,465", "1,234,567")), "v1.10.2"),
        }
        for label, (tree, tag) in cases.items():
            with self.subTest(label), tempfile.TemporaryDirectory() as temp:
                root = write_tree(temp, **tree)
                with self.assertRaises(CheckError):
                    release_check.check(root, tag, "HEAD", None)

    def test_head_off_the_base_is_refused(self):
        with tempfile.TemporaryDirectory() as temp:
            root = write_tree(temp)
            subprocess.run(["git", "branch", "base"], cwd=root, check=True)
            (root / "extra.txt").write_text("x\n", encoding="utf-8")
            subprocess.run(["git", "add", "-A"], cwd=root, check=True)
            subprocess.run(["git", "-c", "user.name=t", "-c", "user.email=t@t", "commit", "-q", "-m", "y"],
                           cwd=root, check=True)
            with self.assertRaises(CheckError):
                release_check.check(root, "v1.10.2", "base", None)

    def test_version_reads_dev_and_refuses_a_mismatch(self):
        with tempfile.TemporaryDirectory() as temp:
            self.assertEqual(release_check.engine_version(write_tree(temp, engine='"1.10.3-dev"',
                                                                     cmake="1.10.3")), "1.10.3-dev")
        with tempfile.TemporaryDirectory() as temp:
            with self.assertRaises(CheckError):
                release_check.engine_version(write_tree(temp, engine='"1.10.3-dev"', cmake="1.10.2"))


if __name__ == "__main__":
    unittest.main()
