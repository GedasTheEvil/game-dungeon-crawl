#!/usr/bin/env python3
"""Plan bookkeeping (docs/plans.md), so agents do not rename files and rewrite links by hand (plan dev-friction-recheck).

    tools/plan.py status              drafts (ready, blocked, on hold) and implemented plans waiting for a play-test
    tools/plan.py move SLUG STAGE     STAGE: draft-sonet, draft-opus, implemented, solved. Renames the plan (git mv),
                                      rewrites every markdown link to it and the links inside it, lists its scenarios
    tools/plan.py links               fixes markdown links to plans that moved, fails on the ones it cannot fix

A plan is found by its slug: <slug>.draft.<model>.md, <slug>.md or solved/<slug>.md in docs/plan/.
"""

import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PLANS = ROOT / "docs/plan"
SOLVED = PLANS / "solved"
NAME = re.compile(r"^(?P<slug>[a-z0-9-]+?)(?:\.draft\.(?P<model>sonet|opus))?\.md$")
LINK = re.compile(r"\]\((?!https?:|#|mailto:)([^)#\s]+\.md)(#[^)\s]*)?\)")
# A link with its text, for a text that names the old path ([plan/x.md](plan/x.md)).
FULL_LINK = re.compile(r"\[([^\]]*)\]\((?!https?:|#|mailto:)([^)#\s]+\.md)(#[^)\s]*)?\)")


def plans():
    """slug -> path of every plan (docs/plan/ and docs/plan/solved/)."""
    out = {}
    for d in (PLANS, SOLVED):
        for p in d.glob("*.md"):
            m = NAME.match(p.name)
            if m:
                out[m.group("slug")] = p
    return out


def slug_of(name):
    m = NAME.match(os.path.basename(name))
    return m.group("slug") if m else None


def markdown_files():
    skip = {".git", "external", "build", "bin", ".claude", "tests"}
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in skip and not d.startswith(".")]
        for f in filenames:
            if f.endswith(".md"):
                yield Path(dirpath) / f


def fix_links(index, report_broken=True):
    """Points every relative markdown link whose file is gone at the plan of the same slug. Returns (fixed, broken)."""
    fixed, broken = 0, []
    for md in markdown_files():
        text = md.read_text()

        def repl(m):
            nonlocal fixed
            label, target, anchor = m.group(1), m.group(2), m.group(3) or ""
            if (md.parent / target).exists():
                return m.group(0)
            slug = slug_of(target)
            if slug not in index:
                broken.append(f"{md.relative_to(ROOT)}: {target}")
                return m.group(0)
            fixed += 1
            new_target = os.path.relpath(index[slug], md.parent)
            if label.strip("`") == target:
                label = label.replace(target, new_target)
            return f"[{label}]({new_target}{anchor})"

        new = FULL_LINK.sub(repl, text)
        if new != text:
            md.write_text(new)
    if report_broken:
        for b in broken:
            print(f"broken link: {b}", file=sys.stderr)
    return fixed, broken


def status_line(path):
    """The plan's text from its first line up to its first section."""
    head = []
    for line in path.read_text().splitlines()[1:]:
        if line.startswith("## "):
            break
        head.append(line)
    return " ".join(head).strip()


def blockers(path, index):
    """Plans its status says it needs first that are not solved yet."""
    out = []
    for sentence in re.split(r"(?<=[.;])\s+", status_line(path)):
        if not re.search(r"\bneeds?\b", sentence, re.I) or re.search(r"needs nothing|does not need", sentence, re.I):
            continue
        for target, _ in LINK.findall(sentence):
            slug = slug_of(target)
            if slug in index and index[slug].parent != SOLVED and index[slug] != path:
                out.append(slug)
    return sorted(set(out))


def status():
    index = plans()
    drafts, implemented = [], []
    for slug, p in sorted(index.items()):
        if p.parent == SOLVED:
            continue
        m = NAME.match(p.name)
        if m.group("model"):
            drafts.append((slug, m.group("model"), p))
        else:
            implemented.append(slug)
    born = {}
    log = subprocess.run(["git", "log", "--diff-filter=A", "--name-only", "--format=%as", "--", "docs/plan"],
                         cwd=ROOT, capture_output=True, text=True).stdout
    date = ""
    for line in log.splitlines():
        if re.match(r"^\d{4}-\d{2}-\d{2}$", line):
            date = line
        elif line and slug_of(line):
            born[slug_of(line)] = date  # older commits come later: the first one wins in the end
    print(f"Implemented, waiting for a play-test ({len(implemented)}): {', '.join(implemented) or '-'}")
    print(f"Solved: {sum(1 for p in index.values() if p.parent == SOLVED)}\n")
    print(f"Drafts ({len(drafts)}), oldest first:")
    rows = []
    for slug, model, p in drafts:
        head = status_line(p)
        need = blockers(p, index)
        if re.search(r"on hold", head, re.I):
            state = "on hold"
        elif need:
            state = "needs " + ", ".join(need)
        else:
            state = "ready"
        written = re.search(r"\d{4}-\d{2}-\d{2}", head)
        rows.append((written.group(0) if written else born.get(slug, "?"), slug, model, state))
    for date, slug, model, state in sorted(rows):
        print(f"  {date}  {model:5}  {slug:34} {state}")
    print("\nOpen questions and holds: read the draft's Status line before picking it.")
    return 0


def move(slug, stage):
    index = plans()
    if slug not in index:
        print(f"no plan {slug}", file=sys.stderr)
        return 1
    names = {"draft-sonet": PLANS / f"{slug}.draft.sonet.md", "draft-opus": PLANS / f"{slug}.draft.opus.md",
             "implemented": PLANS / f"{slug}.md", "solved": SOLVED / f"{slug}.md"}
    if stage not in names:
        print(f"stage: one of {', '.join(names)}", file=sys.stderr)
        return 1
    old, new = index[slug], names[stage]
    if old == new:
        print(f"{slug} is already {new.relative_to(ROOT)}")
        return 0
    # Links inside the plan are relative to its folder: make them absolute before the move.
    text = old.read_text()
    text = LINK.sub(lambda m: f"]({(old.parent / m.group(1)).resolve()}{m.group(2) or ''})", text)
    old.write_text(text)
    subprocess.run(["git", "mv", str(old), str(new)], cwd=ROOT, check=True)
    text = new.read_text()
    text = re.sub(r"\]\((/[^)#\s]+\.md)(#[^)\s]*)?\)",
                  lambda m: f"]({os.path.relpath(m.group(1), new.parent)}{m.group(2) or ''})", text)
    new.write_text(text)
    index[slug] = new
    fixed, broken = fix_links(index)
    print(f"{old.relative_to(ROOT)} -> {new.relative_to(ROOT)}, {fixed} links rewritten")
    if stage == "solved":
        hits = subprocess.run(["git", "grep", "-l", "-w", slug, "--", "tests/scenarios/*.txt", "tests/play/*.txt"],
                              cwd=ROOT, capture_output=True, text=True).stdout.split()
        if hits:
            print("Its scenarios and play scripts (docs/plans.md, Solved: unit tests, or tests/scenarios/solved/):")
            for h in hits:
                print(f"  {h}")
    return 1 if broken else 0


def links():
    fixed, broken = fix_links(plans())
    if fixed:
        print(f"{fixed} links rewritten")
    return 1 if broken else 0


def main(argv):
    if len(argv) >= 1 and argv[0] == "status":
        return status()
    if len(argv) == 3 and argv[0] == "move":
        return move(argv[1], argv[2])
    if len(argv) == 1 and argv[0] == "links":
        return links()
    print(__doc__, file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
