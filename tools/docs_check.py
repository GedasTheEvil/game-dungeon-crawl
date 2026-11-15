#!/usr/bin/env python3
"""Docs drift checks (plan docs-anchor-identifier-checks), run by `make check`.

* Anchors: every `(file.md#anchor)` and `(#anchor)` link must name a heading of its target, as GitHub slugs it.
* Identifiers: every backticked CamelCase, camelCase or UPPER_CASE word must still be in the code (src/, tools/,
  tests/unit/, the makefile), so a renamed or removed name does not live on in the docs.

    tools/docs_check.py [FILE.md ...]

Default files: docs/*.md, readme.md, AGENTS.md, tools/editor/readme.md. Plans are not checked (solved ones are
history), nor glossary.md (tests/unit/docs_test.cpp checks its Code column) or remodeling.md (history). The zone
table of docs/levels.md is checked by docs_test.cpp. Prints one line per problem and exits 1 if there is any.
"""

import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIPPED_DOCS = {"glossary.md", "remodeling.md"}
# Backticked words that are examples or outside names, not the code's: the code style's case names, the launcher
# scripts (LevelEditor, ViewModel), a git setting, a pattern.
ALLOWED = {"CamelCase", "camelBack", "UPPER_CASE", "KEY_CHARS", "LevelEditor", "ViewModel", "ignoreRevsFile", "xxxRect"}
CODE_ROOTS = ["src", "tools", "tests/unit", "makefile"]
CODE_SUFFIXES = (".cpp", ".h", ".py", ".sh", ".txt", ".glsl", ".vert", ".frag")

LINK = re.compile(r"\]\(([^()\s]*?)#([^()\s]+)\)")
HEADING = re.compile(r"^(#{1,6})\s+(.*?)\s*#*\s*$")
SPAN = re.compile(r"`([^`]+)`")
WORD = re.compile(r"\b[A-Za-z_][A-Za-z0-9_]*\b")
IDENTIFIER = re.compile(
    r"[A-Z][A-Z0-9]*(?:_[A-Z0-9]+)+"  # UPPER_CASE
    r"|[a-z][a-z0-9]*[A-Z][A-Za-z0-9]*"  # camelCase
    r"|[A-Z][a-z0-9]+[A-Z][A-Za-z0-9]*"  # CamelCase
)


def default_docs():
    docs = sorted(f"docs/{f}" for f in os.listdir(os.path.join(ROOT, "docs")) if f.endswith(".md"))
    docs = [d for d in docs if os.path.basename(d) not in SKIPPED_DOCS]
    return docs + ["readme.md", "AGENTS.md", "tools/editor/readme.md"]


def prose_lines(path):
    """(line number, text) of the lines outside ``` blocks."""
    with open(path, encoding="utf-8") as f:
        fenced = False
        for n, line in enumerate(f, 1):
            if line.lstrip().startswith("```"):
                fenced = not fenced
                continue
            if not fenced:
                yield n, line


def slug(heading):
    """GitHub's anchor of a heading: lowercase, the markup and punctuation dropped, spaces to dashes."""
    text = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", heading).lower()
    text = re.sub(r"[^\w\- ]", "", text)
    return text.replace(" ", "-")


ANCHORS = {}


def anchors(path):
    if path not in ANCHORS:
        found, seen = set(), {}
        for _, line in prose_lines(path):
            m = HEADING.match(line)
            if not m:
                continue
            s = slug(m.group(2))
            n = seen.get(s, 0)
            seen[s] = n + 1
            found.add(s if n == 0 else f"{s}-{n}")
        ANCHORS[path] = found
    return ANCHORS[path]


def check_anchors(path):
    problems = []
    for n, line in prose_lines(path):
        for target, anchor in LINK.findall(SPAN.sub("", line)):
            if target and not target.endswith(".md"):
                continue
            file = os.path.normpath(os.path.join(os.path.dirname(path), target)) if target else path
            if not os.path.exists(file):
                continue  # a missing file is plan.py links's to report
            if anchor not in anchors(file):
                problems.append(f"{path}:{n}: no heading for #{anchor} in {os.path.relpath(file, ROOT)}")
    return problems


def code_words():
    files = subprocess.run(
        ["git", "ls-files", "--", *CODE_ROOTS], cwd=ROOT, capture_output=True, text=True, check=True
    ).stdout.split()
    words = set()
    for f in files:
        if f == "makefile" or f.endswith(CODE_SUFFIXES):
            with open(os.path.join(ROOT, f), encoding="utf-8", errors="replace") as text:
                words.update(WORD.findall(text.read()))
    return words


def check_identifiers(path, words):
    problems = []
    for n, line in prose_lines(path):
        for span in SPAN.findall(line):
            for word in WORD.findall(span):
                if IDENTIFIER.fullmatch(word) and word not in words and word not in ALLOWED:
                    problems.append(f"{path}:{n}: `{word}` is not in the code (src/, tools/, tests/unit/, makefile)")
    return problems


def main(argv):
    os.chdir(ROOT)
    docs = argv or default_docs()
    words = code_words()
    problems = []
    for doc in docs:
        problems += check_anchors(doc) + check_identifiers(doc, words)
    for p in problems:
        print(p)
    if problems:
        print(f"docs_check: {len(problems)} problem(s)")
        return 1
    print("docs_check: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
