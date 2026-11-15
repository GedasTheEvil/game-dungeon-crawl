#!/usr/bin/env python3
"""Maps the C++ sources for an architecture review: who includes whom, and what nothing uses.

    python3 -I tools/code_map.py graph              includes between directories, as counts
    python3 -I tools/code_map.py edge FROM TO       the includes behind one count (graph's directory names)
    python3 -I tools/code_map.py headers            headers nothing includes, or only other directories do
    python3 -I tools/code_map.py unused             functions a header declares that nothing else names
    make complexity [THRESHOLD=N]                   functions above a cognitive complexity, most complex first
                                                    (code_map.py complexity [--threshold N] SOURCES -- FLAGS)

Reads the quoted includes and the words of every .cpp and .h under src/, tools/ and tests/unit (not external/).
A report to read, not a check: it always exits 0. "unused" is a word count, not a parser: check a hit with grep
before you delete it (a name that another class also has hides; so does one only its own tests call).
"""

import argparse
import collections
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOURCE_DIRS = ("src", "tools", "tests/unit")
INCLUDE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
WORD = re.compile(r"[A-Za-z_]\w*")
COMMENT = re.compile(r"//[^\n]*|/\*.*?\*/", re.S)
STRING = re.compile(r'"(?:\\.|[^"\\\n])*"')
# A function declared in a header: its name, the argument list, then ";" or "{" (or "const", "override", ...).
DECLARATION = re.compile(
    r"(?<![.>:\w])(~?[A-Za-z_]\w*)\s*\((?:[^()]|\([^()]*\))*\)(?:\s*(?:const|noexcept|override)\b)*\s*(;|\{|=)"
)
NOT_A_FUNCTION = frozenset(
    "if for while switch return sizeof static_assert defined alignas alignof decltype catch operator "
    "main constexpr assert nodiscard maybe_unused deprecated".split()
)


def sources():
    files = []
    for top in SOURCE_DIRS:
        files += [p for p in (ROOT / top).rglob("*") if p.suffix in (".cpp", ".h") and p.is_file()]
    return sorted(files)


def group(path):
    """The directory a file counts under: src/sim, tools/editor, tests/unit."""
    return path.parent.relative_to(ROOT).as_posix()


def code(path):
    return path.read_text(encoding="utf-8", errors="replace")


def includes(files):
    """(file, included file) for each quoted include that names one of our files."""
    known = set(files)
    pairs = []
    for path in files:
        for name in INCLUDE.findall(code(path)):
            for base in (path.parent, ROOT / "src", ROOT):
                target = (base / name).resolve()
                if target in known:
                    pairs.append((path, target))
                    break
    return pairs


def is_own_source(includer, header):
    return includer.parent == header.parent and includer.stem == header.stem


def cmd_graph(_args):
    counts = collections.Counter()
    for source, target in includes(sources()):
        if group(source) != group(target):
            counts[group(source), group(target)] += 1
    width = max((len(a) for a, _ in counts), default=0)
    for (source, target), count in sorted(counts.items()):
        both = f"   <-> {counts[target, source]} back" if (target, source) in counts else ""
        print(f"{source:<{width}} -> {target:<{width}} {count:4}{both}")
    mutual = sorted({tuple(sorted(pair)) for pair in counts if pair[::-1] in counts})
    print(f"\n{len(counts)} edges, {len(mutual)} two-way: " + ", ".join(f"{a} <-> {b}" for a, b in mutual))
    print("One edge's includes: tools/code_map.py edge FROM TO")


def cmd_edge(args):
    hits = [
        (source, target)
        for source, target in includes(sources())
        if group(source) == args.source.strip("/") and group(target) == args.target.strip("/")
    ]
    by_target = collections.defaultdict(list)
    for source, target in hits:
        by_target[target].append(source)
    for target, users in sorted(by_target.items(), key=lambda item: -len(item[1])):
        print(f"{target.relative_to(ROOT)} ({len(users)})")
        for user in sorted(users):
            print(f"    {user.relative_to(ROOT)}")
    print(f"\n{len(hits)} includes, {len(by_target)} headers")


def cmd_headers(_args):
    files = sources()
    users = collections.defaultdict(set)
    for source, target in includes(files):
        if not is_own_source(source, target):
            users[target].add(source)
    nobody, elsewhere = [], []
    for header in files:
        if header.suffix != ".h":
            continue
        groups = sorted({group(user) for user in users[header]})
        if not groups:
            nobody.append(header)
        elif group(header) not in groups:
            elsewhere.append((header, groups))
    print(f"Included by nothing but their own .cpp ({len(nobody)}):")
    for header in nobody:
        print(f"    {header.relative_to(ROOT)}")
    print(f"\nIncluded only from other directories ({len(elsewhere)}): a header that may live in the wrong place")
    for header, groups in elsewhere:
        print(f"    {header.relative_to(ROOT)}: {', '.join(groups)}")


def cmd_unused(_args):
    files = sources()
    text = {path: STRING.sub('""', COMMENT.sub(" ", code(path))) for path in files}
    uses = collections.Counter()
    for body in text.values():
        uses.update(WORD.findall(body))
    hits = []
    for header in files:
        if header.suffix != ".h" or not header.is_relative_to(ROOT / "src"):
            continue
        own = text[header] + text.get(header.with_suffix(".cpp"), "")
        own_uses = collections.Counter(WORD.findall(own))
        # Per name: where it first shows, and whether the header holds its body (a call in a body also matches).
        found = {}
        for match in DECLARATION.finditer(text[header]):
            name = match.group(1)
            if name in NOT_A_FUNCTION or name.startswith("~") or name.isupper():
                continue
            first, inline = found.get(name, (match.start(), False))
            found[name] = (first, inline or match.group(2) == "{")
        for name, (start, inline) in found.items():
            # Constructors: the class name is used wherever the type is.
            if re.search(rf"\b(class|struct|enum)\s+{name}\b", text[header]):
                continue
            # Nothing outside names it, and inside it is only declared and defined: a body in the header is one
            # use, a declaration with its definition in the .cpp two. One more would be a call.
            if uses[name] == own_uses[name] and own_uses[name] <= (1 if inline else 2):
                line = text[header].count("\n", 0, start) + 1
                hits.append((header, line, name, own_uses[name]))
    for header, line, name, count in hits:
        print(f"{header.relative_to(ROOT)}:{line}: {name} ({count} in its own .h/.cpp, 0 elsewhere)")
    print(f"\n{len(hits)} candidates. Check each: grep -rnw NAME src tools tests")


# clang-tidy's line for one function (plan complexity-hotspots).
COMPLEXITY = re.compile(r"^(\S+?):(\d+):\d+: warning: function '([^']+)' has cognitive complexity of (\d+)", re.M)


def function_lines(path, line):
    """The lines from the function's first line to its closing brace (comments and strings left out)."""
    text = COMMENT.sub(lambda m: "\n" * m.group(0).count("\n"), code(path))
    text = STRING.sub('""', text)
    lines = text.split("\n")[line - 1:]
    depth, opened = 0, False
    for n, row in enumerate(lines, 1):
        for ch in row:
            if ch == "{":
                depth, opened = depth + 1, True
            elif ch == "}":
                depth -= 1
                if opened and depth == 0:
                    return n
    return 0


def cmd_complexity(args):
    """clang-tidy's readability-function-cognitive-complexity over every source, through the load gate."""
    tidy = os.environ.get("CLANG_TIDY", "clang-tidy")

    def run(src):
        cmd = ["./tools/load_gate.sh", "run", tidy, "--quiet", "--checks=-*,readability-function-cognitive-complexity",
               src, "--", *args.flags]
        return subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True).stdout

    jobs = int(subprocess.run(["./tools/load_gate.sh", "jobs"], cwd=ROOT, capture_output=True, text=True).stdout or 4)
    with ThreadPoolExecutor(jobs + 1) as pool:
        outs = list(pool.map(run, args.sources))
    found = {}  # (file, line) -> (score, name): a header's function shows once per including source
    for out in outs:
        for f, line, name, score in COMPLEXITY.findall(out):
            path = Path(f).resolve()
            found[(path, int(line))] = (int(score), name)
    hits = sorted(((score, path, line, name) for (path, line), (score, name) in found.items() if score > args.threshold),
                  key=lambda h: (-h[0], str(h[1]), h[2]))
    print("| Score | Lines | Function | Where |")
    print("|---|---|---|---|")
    for score, path, line, name in hits:
        where = path.relative_to(ROOT) if path.is_relative_to(ROOT) else path
        print(f"| {score} | {function_lines(path, line)} | `{name}` | {where}:{line} |")
    print(f"\n{len(hits)} functions above {args.threshold} ({len(found)} above clang-tidy's 25), {len(args.sources)} sources")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("graph").set_defaults(run=cmd_graph)
    edge = commands.add_parser("edge")
    edge.add_argument("source", metavar="FROM")
    edge.add_argument("target", metavar="TO")
    edge.set_defaults(run=cmd_edge)
    commands.add_parser("headers").set_defaults(run=cmd_headers)
    commands.add_parser("unused").set_defaults(run=cmd_unused)
    complexity = commands.add_parser("complexity")
    complexity.add_argument("--threshold", type=int, default=25)
    complexity.add_argument("sources", nargs="+")
    complexity.add_argument("flags", nargs=argparse.REMAINDER)
    complexity.set_defaults(run=cmd_complexity)
    argv = sys.argv[1:]
    if "--" in argv:  # complexity SOURCES -- FLAGS: the flags go to clang-tidy as they are
        cut = argv.index("--")
        args = parser.parse_args(argv[:cut])
        args.flags = argv[cut + 1:]
    else:
        args = parser.parse_args()
    args.run(args)
    return 0


if __name__ == "__main__":
    sys.exit(main())
