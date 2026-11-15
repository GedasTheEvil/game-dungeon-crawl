#!/usr/bin/env python3
"""Where is X: the rows of docs/where-is-x.md that match, so an agent reads a few rows, not the whole map.

    tools/where.py TERM...      the rows holding every term (feature or files, case-insensitive), as "feature: files"
    tools/where.py --list       the features only, one per line

Paths without a top directory are under src/. Exits 1 when no row matches (try fewer or other terms, or --list).
"""

import argparse
import sys
from pathlib import Path

MAP = Path(__file__).resolve().parent.parent / "docs" / "where-is-x.md"


def rows(text):
    """(feature, files) per table row, the header and the separator left out."""
    found = []
    for line in text.splitlines():
        if not line.startswith("| ") or line.startswith("| Feature | Files |"):
            continue
        cells = [c.strip() for c in line.strip().strip("|").split(" | ", 1)]
        if len(cells) == 2:
            found.append((cells[0], cells[1]))
    return found


def matches(table, terms):
    terms = [t.lower() for t in terms]
    return [r for r in table if all(t in f"{r[0]} {r[1]}".lower() for t in terms)]


def main(argv):
    parser = argparse.ArgumentParser(description="Rows of docs/where-is-x.md matching every term.")
    parser.add_argument("terms", nargs="*")
    parser.add_argument("--list", action="store_true", help="print the features only")
    args = parser.parse_args(argv)
    table = rows(MAP.read_text(encoding="utf-8"))
    if args.list:
        for feature, _ in table:
            print(feature)
        return 0
    if not args.terms:
        parser.error("give a term, or --list")
    hits = matches(table, args.terms)
    for feature, files in hits:
        print(f"{feature}: {files}")
    if not hits:
        print(f"no row holds all of: {' '.join(args.terms)} (fewer terms, or --list)", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
