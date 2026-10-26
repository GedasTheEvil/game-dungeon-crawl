#!/usr/bin/env python3
"""Generate rebuilt campaign levels again from their seed table (plan denser-levels, tools/level/rebuild.txt).

    tools/level/rebuild.py LEVEL...    writes content/levels/lvlLEVEL with bin/levelgen and the level's arguments

Run from anywhere; needs bin/levelgen (make). Overwrites the level file: the polish done since is lost (git diff).
"""

import shlex
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
TABLE = ROOT / "tools/level/rebuild.txt"


def table():
    levels = {}
    for line in TABLE.read_text().splitlines():
        line = line.strip()
        if line and not line.startswith("#"):
            words = shlex.split(line)
            levels[int(words[0])] = words[1:]
    return levels


def main(argv):
    if not argv:
        print(__doc__.strip())
        return 2
    levels = table()
    for arg in argv:
        level = int(arg)
        if level not in levels:
            print(f"level {level} is not in {TABLE.relative_to(ROOT)}")
            return 1
        out = ROOT / f"content/levels/lvl{level}"
        subprocess.run([ROOT / "bin/levelgen", *levels[level], out], cwd=ROOT, check=True)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
