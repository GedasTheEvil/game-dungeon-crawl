#!/usr/bin/env python3
"""Tests of tools/plan.py in a throwaway git repo (make check runs it): names, moves, solve, reject, verdict, lint."""

import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOL = Path(__file__).resolve().parent / "plan.py"
sys.path.insert(0, str(TOOL.parent))
import plan  # noqa: E402


def write(root, name, text):
    p = root / name
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text)


class Names(unittest.TestCase):
    def test_parse(self):
        self.assertEqual(plan.parse_name("a-b.draft.opus.md"), ("a-b", None, "opus"))
        self.assertEqual(plan.parse_name("a-b.bug.draft.sonet.md"), ("a-b", "bug", "sonet"))
        self.assertEqual(plan.parse_name("a-b.tooling.md"), ("a-b", "tooling", None))
        self.assertEqual(plan.parse_name("a-b.md"), ("a-b", None, None))
        self.assertIsNone(plan.parse_name("a-b.draft.md"))
        self.assertIsNone(plan.parse_name("BOARD.md"))

    def test_verdict(self):
        v = plan.parse_verdict("ok a\n✗ b: too slow; no b: no sound\n✓ c")
        self.assertEqual(v, {"a": (True, []), "b": (False, ["too slow", "no sound"]), "c": (True, [])})
        with self.assertRaises(ValueError):
            plan.parse_verdict("ok a; no a: bad")
        with self.assertRaises(ValueError):
            plan.parse_verdict("maybe a")


class Repo(unittest.TestCase):
    def setUp(self):
        self.root = Path(tempfile.mkdtemp())
        (self.root / "tools").mkdir()
        shutil.copy(TOOL, self.root / "tools/plan.py")
        write(self.root, "docs/plan/fix-x.bug.draft.opus.md",
              "# Fix x\n\nStatus: draft 2026-01-01, size S, needs nothing first.\n\n## Repro\n\n[s](../../tests/scenarios/fix_x.txt)\n")
        write(self.root, "docs/plan/feat-y.draft.sonet.md",
              "# Y\n\nStatus: draft 2026-01-02, size M, needs [fix-x](fix-x.bug.draft.opus.md) first.\n\n## Idea\n")
        write(self.root, "docs/plan/tool-z.tooling.draft.opus.md",
              "# Z\n\nStatus: draft 2026-01-03, size L, prio high, needs nothing first.\n\n## Plan\n")
        write(self.root, "docs/notes.md", "See [x](plan/fix-x.bug.draft.opus.md), `tests/scenarios/fix_x.txt`.\n")
        write(self.root, "src/x.cpp", "// X (plan fix-x)\n")
        write(self.root, "tests/unit/x_test.cpp", "// plan fix-x\n")
        write(self.root, "tests/scenarios/fix_x.txt", "# Plan fix-x: the picture\nscreenshot a\n")
        write(self.root, "tests/play/fix-x.txt", "# Plan fix-x\nlevel 1\n")
        write(self.root, "content/levels/lvl1", "level\n")
        self.git("init", "-q")
        self.git("-c", "user.name=t", "-c", "user.email=t@t", "add", ".")
        self.git("-c", "user.name=t", "-c", "user.email=t@t", "commit", "-qm", "init")

    def tearDown(self):
        shutil.rmtree(self.root)

    def git(self, *args):
        return subprocess.run(["git", *args], cwd=self.root, check=True, capture_output=True, text=True).stdout

    def run_tool(self, *args, code=0):
        r = subprocess.run([sys.executable, "-I", str(self.root / "tools/plan.py"), *args], cwd=self.root,
                           capture_output=True, text=True)
        self.assertEqual(r.returncode, code, r.stdout + r.stderr)
        return r.stdout + r.stderr

    def read(self, name):
        return (self.root / name).read_text()

    def test_status_and_next(self):
        out = self.run_tool("status")
        self.assertIn("Bug drafts (1)", out)
        self.assertIn("needs fix-x", out)
        self.assertIn("(code names it already)", out)
        self.assertIn("Next: fix-x (bug", self.run_tool("next"))
        self.assertIn("Next: tool-z", self.run_tool("next", "--kind", "tooling"))
        self.assertIn("nothing ready", self.run_tool("next", "--size", "S", "--kind", "feature", code=1))

    def test_lint(self):
        self.run_tool("lint")
        write(self.root, "docs/plan/bad.draft.opus.md", "# Bad\n\nStatus: draft 2026-01-01.\n")
        write(self.root, "docs/plan/solved/old.draft.opus.md", "# Old\n\nStatus: solved.\n")
        out = self.run_tool("lint", code=1)
        self.assertIn("no size", out)
        self.assertIn("moved by hand", out)

    def test_implement_reject_verdict_solve(self):
        out = self.run_tool("move", "fix-x", "implemented", code=1)
        self.assertIn("Play-test", out)  # no checklist: refused
        with open(self.root / "docs/plan/fix-x.bug.draft.opus.md", "a") as f:
            f.write("\n## Play-test\n\n* Look at x.\n")
        self.run_tool("move", "fix-x", "implemented")
        self.assertIn("Status: implemented ", self.read("docs/plan/fix-x.bug.md"))
        self.assertIn("plan/fix-x.bug.md", self.read("docs/notes.md"))
        self.assertIn("bin/game --play tests/play/fix-x.txt", self.run_tool("playtest"))
        self.run_tool("verdict", "no fix-x: still broken")
        self.assertIn("- [ ] still broken", self.read("docs/plan/fix-x.bug.md"))
        self.assertIn("Rework after a play-test (1)", self.run_tool("status"))
        self.run_tool("reject", "fix-x", "and slow")
        self.assertIn("### Round 2", self.read("docs/plan/fix-x.bug.md"))
        out = self.run_tool("verdict", "ok fix-x")
        self.assertIn("Commit message:", out)
        solved = self.read("docs/plan/solved/fix-x.bug.md")
        self.assertIn("Status: solved ", solved)
        self.assertIn("(play-tested by the user)", solved)
        self.assertTrue((self.root / "tests/scenarios/solved/fix_x.txt").exists())
        self.assertFalse((self.root / "tests/play/fix-x.txt").exists())
        self.assertIn("plan/solved/fix-x.bug.md", self.read("docs/notes.md"))
        self.assertIn("`tests/scenarios/solved/fix_x.txt`", self.read("docs/notes.md"))
        self.assertIn("(../../../tests/scenarios/solved/fix_x.txt)", solved)
        self.assertNotIn("needs fix-x", self.run_tool("status"))

    def test_tooling_goes_straight_to_solved(self):
        self.run_tool("move", "tool-z", "implemented")
        self.assertTrue((self.root / "docs/plan/solved/tool-z.tooling.md").exists())

    def test_kind_and_model(self):
        self.run_tool("move", "feat-y", "kind-bug")
        self.run_tool("move", "feat-y", "draft-opus")
        self.assertTrue((self.root / "docs/plan/feat-y.bug.draft.opus.md").exists())

    def test_hold_and_needs(self):
        self.run_tool("hold", "tool-z", "waits for the user")
        self.assertIn("on hold", self.run_tool("status"))
        self.run_tool("unhold", "tool-z")
        self.assertNotIn("On hold", self.read("docs/plan/tool-z.tooling.draft.opus.md"))
        self.run_tool("needs", "tool-z", "fix-x")
        self.assertIn("needs [fix-x](fix-x.bug.draft.opus.md) first", self.read("docs/plan/tool-z.tooling.draft.opus.md"))
        self.run_tool("needs", "tool-z", "feat-y")
        self.assertIn("feat-y", self.run_tool("status").split("tool-z")[1].split("\n")[0])
        self.run_tool("unneed", "tool-z", "fix-x")
        self.run_tool("unneed", "tool-z", "feat-y")
        self.assertIn("needs nothing first", self.read("docs/plan/tool-z.tooling.draft.opus.md"))

    def test_new(self):
        self.run_tool("new", "add-w", "--kind", "feature", "--model", "sonet", "--size", "S", "Add", "w")
        text = self.read("docs/plan/add-w.feature.draft.sonet.md")
        self.assertTrue(text.startswith("# Add w\n"))
        self.assertIn("size S", text)
        self.assertIn("## Play-test", text)
        self.run_tool("lint")
        self.run_tool("new", "add-w", "--kind", "bug", "--model", "opus", "--size", "S", "Again", code=1)

    def test_find(self):
        out = self.run_tool("find", "IDEA", "y")
        self.assertIn("draft feat-y: Y", out)
        self.assertNotIn("draft fix-x", self.run_tool("find", "needs", "first", "feat"))
        self.run_tool("move", "tool-z", "implemented")
        self.assertIn("solved tool-z: Z", self.run_tool("find", "prio high"))
        self.assertIn("no plan has", self.run_tool("find", "nowhere", code=1))

    def test_board(self):
        self.run_tool("board")
        self.assertIn("## Ready: bug (1)", self.read("docs/plan/BOARD.md"))
        self.run_tool("lint")


if __name__ == "__main__":
    unittest.main(verbosity=0)
