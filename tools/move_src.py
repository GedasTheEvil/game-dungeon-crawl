#!/usr/bin/env python3
"""Moves source files (git mv) and fixes every path that names them (plan src-topic-subdirs).

    tools/move_src.py [--dry-run] OLD=NEW...
    tools/move_src.py [--dry-run] -f MOVES     one OLD NEW pair per line (# comments, blank lines skipped)

OLD and NEW are paths from the repo root. A stem (src/level/scroll_bar) moves its .h and .cpp together; a file name
(src/level/gameplay_traps.h) moves that file. NEW may rename (src/level/input -> src/level/input/keys).

1. Quoted includes in src/, tools/ and tests/ (.cpp, .h): each one that names a moved file, or sits in a moved file,
   is written again relative to the including file's new place.
2. git mv.
3. Path mentions in every other tracked text file and in the code's comments: src/level/scroll_bar.h,
   level/scroll_bar.cpp, ../src/level/scroll_bar.h (a doc link), $(BUILD)/src/level/scroll_bar.o, level/scroll_bar.*.
   Solved and won't-do plans are history: left alone. A mention without a directory (scroll_bar.h) is left too.
Prints the files it changed. Then grep for what it cannot see: globs (level/gameplay_*.h), Python path joins.
"""

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CODE_DIRS = ("src", "tools", "tests")
HISTORY = ("docs/plan/solved/", "docs/plan/wont-do/")
INCLUDE = re.compile(r'^(\s*#\s*include\s+")([^"]+)(")', re.M)


def git(*args):
    return subprocess.run(["git", *args], cwd=ROOT, check=True, capture_output=True, text=True).stdout


def read_moves(pairs):
    """{old file: new file}, both relative to the root, every stem expanded to its .h and .cpp."""
    moves = {}
    for old, new in pairs:
        old, new = old.strip().rstrip("/"), new.strip().rstrip("/")
        if old.endswith((".h", ".cpp")):
            if not (ROOT / old).is_file():
                sys.exit(f"move_src: no file {old}")
            moves[old] = new
            continue
        found = [ext for ext in (".h", ".cpp") if (ROOT / (old + ext)).is_file()]
        if not found:
            sys.exit(f"move_src: no {old}.h or {old}.cpp")
        for ext in found:
            moves[old + ext] = new + ext
    for new in moves.values():
        if (ROOT / new).exists() and new not in moves:
            sys.exit(f"move_src: {new} exists")
    return moves


def fix_includes(moves, dry_run):
    changed = []
    files = [f for f in git("ls-files", "--cached", "--others", "--exclude-standard", "--", *CODE_DIRS).split()
             if f.endswith((".cpp", ".h"))]
    for name in files:
        path = ROOT / name
        if not path.is_file():
            continue
        new_dir = os.path.dirname(moves.get(name, name))
        text = path.read_text(encoding="utf-8")

        def fix(m):
            target = os.path.normpath(os.path.join(os.path.dirname(name), m.group(2)))
            if name not in moves and target not in moves:
                return m.group(0)
            return m.group(1) + os.path.relpath(moves.get(target, target), new_dir) + m.group(3)

        fixed = INCLUDE.sub(fix, text)
        if fixed != text:
            changed.append(name)
            if not dry_run:
                path.write_text(fixed, encoding="utf-8")
    return changed


def mention_pattern(moves):
    """A regex for the moved stems as text names them, and {old stem under src/: new stem under src/}."""
    stems = {}
    for old, new in moves.items():
        if old.startswith("src/") and new.startswith("src/"):
            stems[os.path.splitext(old)[0][4:]] = os.path.splitext(new)[0][4:]
    alternatives = "|".join(re.escape(s) for s in sorted(stems, key=len, reverse=True))
    # src/x (also after $(BUILD)/ or ../), or x after ../ or after no path at all (not tools/level/x).
    prefix = r"(?P<pre>(?<![\w.-])(?:\.\./)*src/|(?<![\w./-])(?:\.\./)*)"
    return re.compile(prefix + rf"(?P<stem>{alternatives})(?=\.(?:h|cpp|o|d)\b|\.\*|\.\{{)"), stems


def fix_mentions(moves, dry_run):
    pattern, stems = mention_pattern(moves)
    if not stems:
        return []
    changed = []
    for name in git("ls-files").split():
        if name.startswith(HISTORY) or name == "tools/move_src.py":  # its examples are old paths
            continue
        path = ROOT / name
        if not path.is_file() or path.is_symlink():
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        lines = text.splitlines(keepends=True)
        for i, line in enumerate(lines):
            if not INCLUDE.match(line):
                lines[i] = pattern.sub(lambda m: m.group("pre") + stems[m.group("stem")], line)
        fixed = "".join(lines)
        if fixed != text:
            changed.append(name)
            if not dry_run:
                path.write_text(fixed, encoding="utf-8")
    return changed


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--dry-run", action="store_true", help="print what would change, change nothing")
    parser.add_argument("-f", dest="file", help="a file of OLD NEW lines")
    parser.add_argument("moves", nargs="*", metavar="OLD=NEW")
    args = parser.parse_args(argv)
    pairs = [m.split("=", 1) for m in args.moves]
    if args.file:
        for line in Path(args.file).read_text(encoding="utf-8").splitlines():
            line = line.split("#", 1)[0].split()
            if line:
                pairs.append(line)
    if not pairs or any(len(p) != 2 for p in pairs):
        parser.error("give OLD=NEW pairs or -f MOVES")
    moves = read_moves(pairs)
    includes = fix_includes(moves, args.dry_run)
    for old, new in moves.items():
        print(f"{'would move' if args.dry_run else 'moved'} {old} -> {new}")
        if not args.dry_run:
            (ROOT / new).parent.mkdir(parents=True, exist_ok=True)
            git("mv", old, new)
    mentions = fix_mentions(moves, args.dry_run)
    print(f"includes fixed in {len(includes)} files, mentions in {len(mentions)}:")
    for name in sorted(set(includes) | set(mentions)):
        print(f"    {moves.get(name, name)}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
