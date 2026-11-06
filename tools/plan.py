#!/usr/bin/env python3
"""Plan bookkeeping (docs/plans.md), so agents do not rename files, edit Status lines and rewrite links by hand.

  Reading
    status                     open plans: rework, waiting for a play-test, drafts by kind (ready, blocked, on hold)
    next [--size S|M|L] [--kind K] [--model M]
                               the plan to do next: rework first, then bugs, priority, age. --size: at most that big
    playtest [SLUG]            per plan waiting for a play-test: the command, its checklist, days waiting, warnings
    board                      writes docs/plan/BOARD.md (not tracked): the plans as a board, for a read by hand
    find TERM...               every plan (drafts, implemented, solved, won't do) whose name, title, Status line or
                               body has all the TERMs (case-insensitive): "STAGE SLUG: title" and the first
                               matching line
  Writing
    new SLUG --kind K --model M --size S [--prio P] TITLE
                               a draft skeleton: docs/plan/SLUG.K.draft.M.md
    move SLUG STAGE            STAGE: draft-sonet, draft-opus, implemented, solved, wont-do, kind-bug, kind-feature,
                               kind-tooling. Renames (git mv), rewrites the links and the Status line. implemented:
                               runs done-check first, refuses on a failure (--force moves anyway); a tooling plan
                               goes straight to solved
    solve SLUG... [--playtested]
                               move solved, park the scenarios that are about it in tests/scenarios/solved/, remove
                               its play script, print a commit message
    reject SLUG NOTE...        a failed play-test: a "Play-test feedback" round with one "- [ ]" item per NOTE
    verdict TEXT...            the user's play-test reply, one item a line (or ";"): "ok SLUG" / "✓ SLUG" solves it
                               (play-tested), "no SLUG: note" / "✗ SLUG: note" rejects it
    hold SLUG REASON | unhold SLUG | needs SLUG OTHER | unneed SLUG OTHER
                               edit the draft's Status line
  Checks
    done-check SLUG            what an implemented plan must have (code and test that name it, play command,
                               checklist), and the reminders
    lint                       file names against Status lines, sizes, needs on won't-do plans (make check)
    links                      fixes markdown links to plans that moved, fails on the ones it cannot fix
                               (not in solved/ and wont-do/ plans: those are history, their links may rot)

A plan is found by its slug: SLUG[.KIND][.draft.MODEL].md in docs/plan/, solved/ or wont-do/. KIND: bug, feature,
tooling (none: an old plan, a feature).
"""

import os
import re
import subprocess
import sys
from dataclasses import dataclass
from datetime import date
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PLANS = ROOT / "docs/plan"
SOLVED = PLANS / "solved"
WONT_DO = PLANS / "wont-do"
CLOSED = (SOLVED, WONT_DO)
BOARD = PLANS / "BOARD.md"
KINDS = ("bug", "feature", "tooling")
MODELS = ("opus", "sonet")
SIZES = ("S", "M", "L")
PRIOS = ("high", "normal", "low")
NAME = re.compile(r"^(?P<slug>[a-z0-9-]+?)(?:\.(?P<kind>bug|feature|tooling))?(?:\.draft\.(?P<model>sonet|opus))?\.md$")
LINK = re.compile(r"\]\((?!https?:|#|mailto:)([^)#\s]+\.md)(#[^)\s]*)?\)")
# Any relative link (a plan, a scenario, a level, an image), rebased when its plan changes folder.
REL_LINK = re.compile(r"\]\((?!https?:|#|mailto:|/)([^)#\s]+)(#[^)\s]*)?\)")
# A link with its text, for a text that names the old path ([plan/x.md](plan/x.md)).
FULL_LINK = re.compile(r"\[([^\]]*)\]\((?!https?:|#|mailto:)([^)#\s]+\.md)(#[^)\s]*)?\)")
DATE = re.compile(r"\d{4}-\d{2}-\d{2}")
SIZE = re.compile(r"\bsize:?\s+([SML])\b", re.I)
PRIO = re.compile(r"\bprio(?:rity)?:?\s+(high|normal|low)\b", re.I)
FEEDBACK = "## Play-test feedback"
CHECKLIST = "## Play-test"
CODE_DIRS = ("src", "content", "tests", "tools")


@dataclass
class Plan:
    slug: str
    path: Path
    kind: str | None  # None: an old plan without a kind in its name (a feature)
    model: str | None  # set on drafts only

    @property
    def stage(self):
        if self.path.parent == SOLVED:
            return "solved"
        if self.path.parent == WONT_DO:
            return "wont-do"
        return "draft" if self.model else "implemented"

    @property
    def kind_or_default(self):
        return self.kind or "feature"

    @property
    def open(self):
        return self.stage in ("draft", "implemented")

    def text(self):
        return self.path.read_text()

    def head(self):
        return status_line(self.path)

    def size(self):
        m = SIZE.search(self.head())
        return m.group(1).upper() if m else "?"

    def prio(self):
        m = PRIO.search(self.head())
        return m.group(1).lower() if m else "normal"


def parse_name(name):
    """(slug, kind, model) of a plan file name, or None."""
    m = NAME.match(os.path.basename(name))
    return (m.group("slug"), m.group("kind"), m.group("model")) if m else None


def file_for(slug, stage, kind, model):
    part = f".{kind}" if kind else ""
    if stage == "draft":
        return PLANS / f"{slug}{part}.draft.{model}.md"
    folder = {"implemented": PLANS, "solved": SOLVED, "wont-do": WONT_DO}[stage]
    return folder / f"{slug}{part}.md"


def plan_files():
    for d in (PLANS, *CLOSED):
        for p in sorted(d.glob("*.md")):
            if p != BOARD:
                yield p


def plans():
    """slug -> Plan of every plan (docs/plan/, docs/plan/solved/ and docs/plan/wont-do/)."""
    out = {}
    for p in plan_files():
        parsed = parse_name(p.name)
        if parsed:
            out[parsed[0]] = Plan(parsed[0], p, parsed[1], parsed[2])
    return out


def slug_of(name):
    parsed = parse_name(name)
    return parsed[0] if parsed else None


def git(*args, check=False):
    return subprocess.run(["git", *args], cwd=ROOT, capture_output=True, text=True, check=check).stdout


def rel(path):
    return path.relative_to(ROOT)


def link_to(target, origin):
    """A markdown link from the file origin to the plan target."""
    return f"[{target.slug}]({os.path.relpath(target.path, origin.parent)})"


def markdown_files():
    """Every markdown file whose links are kept up to date. Closed plans (solved, won't do) are history: not checked."""
    skip = {".git", "external", "build", "bin", ".claude", "tests"}
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d not in skip and not d.startswith(".")
                       and Path(dirpath) / d not in CLOSED]
        for f in filenames:
            if f.endswith(".md") and Path(dirpath) / f != BOARD:
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
                broken.append(f"{rel(md)}: {target}")
                return m.group(0)
            fixed += 1
            new_target = os.path.relpath(index[slug].path, md.parent)
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


# --- Status line --------------------------------------------------------------------------------------------------


def status_line(path):
    """The plan's text from its first line up to its first section."""
    head = []
    for line in path.read_text().splitlines()[1:]:
        if line.startswith("## "):
            break
        head.append(line)
    return " ".join(head).strip()


def status_span(lines):
    """(first, end) line numbers of the "Status:" paragraph, or None."""
    i = next((k for k, line in enumerate(lines) if line.startswith("Status:")), None)
    if i is None:
        return None
    end = next((k for k in range(i, len(lines)) if not lines[k].strip()), len(lines))
    return i, end


def status_text(path):
    lines = path.read_text().split("\n")
    span = status_span(lines)
    return " ".join(lines[span[0]:span[1]])[len("Status:"):].strip() if span else None


def mark_stage(path, prefix, already):
    """Turns the Status paragraph into "Status: PREFIX. Was: ...", unless it already starts with the word `already`."""
    lines = path.read_text().split("\n")
    span = status_span(lines)
    if span is None:
        return False
    i = span[0]
    if re.match(rf"Status:\s*{already}\b", lines[i], re.I):
        return False
    lines[i] = f"Status: {prefix}. Was: " + lines[i][len("Status:"):].lstrip()
    path.write_text("\n".join(lines))
    return True


def mark_solved(path, day, note=""):
    """Turns the first Status line into "Status: solved DAY[ NOTE]. Was: ...", unless it already says solved."""
    lines = path.read_text().split("\n")
    span = status_span(lines)
    if span is None or re.search(r"\bsolved\b", " ".join(lines[span[0]:span[1]]), re.I):
        return False
    return mark_stage(path, f"solved {day}{note}", "solved")


def edit_status(path, fn):
    """Rewrites the Status paragraph (joined to one line, without "Status: ") through fn."""
    lines = path.read_text().split("\n")
    span = status_span(lines)
    if span is None:
        raise SystemExit(f"{rel(path)}: no Status line")
    i, end = span
    old = " ".join(line.strip() for line in lines[i:end])[len("Status:"):].strip()
    new = fn(old)
    if new == old:
        return False
    lines[i:end] = [f"Status: {new}"]
    path.write_text("\n".join(lines))
    return True


def sentences(text):
    return re.split(r"(?<=[.;])\s+", text)


def blockers(plan, index):
    """Plans its status says it needs first that are not solved yet (a won't-do one is marked: it never will be)."""
    out = []
    for sentence in sentences(plan.head()):
        if not re.search(r"\bneeds?\b", sentence, re.I) or re.search(r"needs nothing|does not need", sentence, re.I):
            continue
        if re.match(r"was:", sentence, re.I):
            continue
        for target, _ in LINK.findall(sentence):
            slug = slug_of(target)
            if slug in index and index[slug].stage != "solved" and index[slug].path != plan.path:
                out.append(f"{slug} (won't do)" if index[slug].stage == "wont-do" else slug)
    return sorted(set(out))


def on_hold(plan):
    return bool(re.search(r"\bon hold\b", plan.head(), re.I))


def written(plan, born):
    m = DATE.search(plan.head())
    return m.group(0) if m else born.get(plan.slug, "?")


def birthdays():
    """slug -> the date its first file was added."""
    born = {}
    day = ""
    for line in git("log", "--diff-filter=A", "--name-only", "--format=%as", "--", "docs/plan").splitlines():
        if DATE.fullmatch(line):
            day = line
        elif line and slug_of(line):
            born[slug_of(line)] = day  # older commits come later: the first one wins in the end
    return born


def draft_state(plan, index):
    if on_hold(plan):
        return "on hold"
    need = blockers(plan, index)
    return "needs " + ", ".join(need) if need else "ready"


# --- Play-test ----------------------------------------------------------------------------------------------------


def section(text, title):
    """The lines of the section `title` (a "## " heading), or None."""
    lines = text.split("\n")
    i = next((k for k, line in enumerate(lines) if line.strip() == title), None)
    if i is None:
        return None
    end = next((k for k in range(i + 1, len(lines)) if lines[k].startswith("## ")), len(lines))
    return lines[i + 1:end]


def open_feedback(plan):
    """The play-test feedback items not ticked yet."""
    body = section(plan.text(), FEEDBACK) or []
    return [line.strip()[len("- [ ]"):].strip() for line in body if line.strip().startswith("- [ ]")]


def play_script(slug):
    p = ROOT / "tests/play" / f"{slug}.txt"
    return p if p.exists() else None


def play_command(plan):
    script = play_script(plan.slug)
    if script:
        return f"bin/game --play {rel(script)}"
    m = re.search(r"`(bin/game [^`]+)`", plan.text())
    return m.group(1) if m else None


def checklist(plan):
    body = section(plan.text(), CHECKLIST)
    return [line for line in body if line.strip()] if body else []


def commit_time(path):
    out = git("log", "-1", "--format=%ct", "--", str(path)).strip()
    return int(out) if out else None


def level_file(arg):
    if arg.isdigit():
        return ROOT / "content/levels" / f"lvl{arg}"
    p = ROOT / arg
    if p.is_file():
        return p
    return p.with_name(p.name + ".txt") if p.with_name(p.name + ".txt").is_file() else None


def script_warnings(script):
    """A level a play script loads that is gone or changed after the script."""
    out = []
    since = commit_time(script)
    for line in script.read_text().splitlines():
        m = re.match(r"\s*level\s+(\S+)", line)
        if not m or m.group(1).startswith("gen:"):
            continue
        lvl = level_file(m.group(1))
        if lvl is None:
            out.append(f"its level {m.group(1)} is gone")
        elif since and (commit_time(lvl) or 0) > since:
            out.append(f"its level {rel(lvl)} changed after the script: check the script still sets the scene")
    return out


def waiting_since(plan):
    m = re.match(r"implemented (\d{4}-\d{2}-\d{2})", status_text(plan.path) or "")
    if m:
        return m.group(1)
    out = git("log", "-1", "--format=%as", "--", str(plan.path)).strip()
    return out or date.today().isoformat()


def days_since(day):
    try:
        return (date.today() - date.fromisoformat(day)).days
    except ValueError:
        return 0


def playtest(slug=None):
    index = plans()
    todo = [p for p in index.values() if p.stage == "implemented" and (slug is None or p.slug == slug)]
    if slug and not todo:
        print(f"{slug} is not waiting for a play-test", file=sys.stderr)
        return 1
    for p in sorted(todo, key=lambda p: waiting_since(p)):
        day = waiting_since(p)
        fb = open_feedback(p)
        print(f"{p.slug} ({p.kind_or_default}), implemented {day}, waiting {days_since(day)} d"
              + (f", REWORK: {len(fb)} feedback item(s) open" if fb else ""))
        cmd = play_command(p)
        print(f"  {cmd}" if cmd else "  WARNING: no play command (tests/play/<slug>.txt or `bin/game --level N`)")
        script = play_script(p.slug)
        for w in script_warnings(script) if script else []:
            print(f"  WARNING: {w}")
        items = checklist(p)
        if items:
            for line in items:
                print(f"  {line}")
        else:
            print(f"  WARNING: no \"{CHECKLIST}\" checklist in {rel(p.path)}")
        for item in fb:
            print(f"  open feedback: {item}")
        print()
    print("Reply per plan: \"ok SLUG\" or \"no SLUG: what is wrong\" (tools/plan.py verdict).")
    return 0


# --- Done check ---------------------------------------------------------------------------------------------------


def files_naming(slug, dirs=CODE_DIRS):
    return [f for f in git("grep", "-l", "-w", "-e", slug, "--", *dirs).split() if not f.startswith("tests/play/")]


def done_check(plan, quiet=False):
    """(failures, warnings) of an implemented (or about to be) plan."""
    fails, warns = [], []
    if status_text(plan.path) is None:
        fails.append("no Status line")
    code = files_naming(plan.slug)
    tests = [f for f in code if f.startswith(("tests/unit/", "tests/scenarios/"))]
    if not code:
        warns.append(f"no code, level or test names it (a comment \"(plan {plan.slug})\" where it is built)")
    if plan.kind_or_default == "bug" and not tests:
        fails.append("a bug fix comes with a test that names the plan (tests/unit or tests/scenarios), "
                     "the one that failed before the fix")
    elif not tests and plan.kind_or_default != "tooling":
        warns.append("no unit test or scenario names it")
    if plan.kind_or_default != "tooling":
        if not play_command(plan):
            fails.append("no play command: write tests/play/<slug>.txt or put `bin/game --level N` in the plan")
        if not checklist(plan):
            fails.append(f"no \"{CHECKLIST}\" section: 2-5 lines, what the user does and what they should see")
        script = play_script(plan.slug)
        warns += script_warnings(script) if script else []
    if not quiet:
        for f in fails:
            print(f"FAIL  {f}")
        for w in warns:
            print(f"warn  {w}")
        print("todo  every point of the draft done, or reported as left out\n"
              "todo  a new thing the player meets: its journal or field note; a new term: its glossary row\n"
              "todo  a base monster with a new ability: its boss gets it too (AGENTS.md, Boss abilities)\n"
              "todo  a new feature: its row in docs/development.md, Where is X")
    return fails, warns


# --- Commands -----------------------------------------------------------------------------------------------------


def status():
    index = plans()
    born = birthdays()
    implemented = [p for p in index.values() if p.stage == "implemented"]
    rework = [p for p in implemented if open_feedback(p)]
    waiting = [p for p in implemented if p not in rework]
    cited = set(git("grep", "-o", "-h", "-w", *sum((["-e", p.slug] for p in index.values() if p.stage == "draft"),
                                                     []), "--", *CODE_DIRS).split()) if any(
        p.stage == "draft" for p in index.values()) else set()
    if rework:
        print(f"Rework after a play-test ({len(rework)}): "
              + ", ".join(f"{p.slug} ({len(open_feedback(p))} open)" for p in rework))
    print(f"Implemented, waiting for a play-test ({len(waiting)}): "
          + (", ".join(f"{p.slug} ({days_since(waiting_since(p))} d)" for p in
                       sorted(waiting, key=waiting_since)) or "-"))
    print(f"Solved: {sum(1 for p in index.values() if p.stage == 'solved')}, "
          f"won't do: {sum(1 for p in index.values() if p.stage == 'wont-do')}")
    drafts = [p for p in index.values() if p.stage == "draft"]
    for kind in KINDS:
        rows = sorted(((written(p, born), p) for p in drafts if p.kind_or_default == kind),
                      key=lambda r: (r[0], r[1].slug))
        if not rows:
            continue
        print(f"\n{kind.capitalize()} drafts ({len(rows)}), oldest first:")
        for day, p in rows:
            prio = "" if p.prio() == "normal" else p.prio()
            note = "  (code names it already)" if p.slug in cited else ""
            print(f"  {day}  {p.model:5}  {p.size()}  {prio:4}  {p.slug:34} {draft_state(p, index)}{note}")
    print("\nSize: S an hour or less, M a few hours, L a day or more. Next pick: tools/plan.py next; "
          "play-test commands: tools/plan.py playtest.\nOpen questions: read the draft's Status line before picking it.")
    return 0


def next_plan(args):
    opts = flags(args, {"--size": SIZES, "--kind": KINDS, "--model": MODELS})
    if opts is None:
        return 2
    index = plans()
    born = birthdays()
    rework = [p for p in index.values() if p.stage == "implemented" and open_feedback(p)]
    ready = [p for p in index.values() if p.stage == "draft" and draft_state(p, index) == "ready"]
    if "--kind" in opts:
        rework = [p for p in rework if p.kind_or_default == opts["--kind"]]
        ready = [p for p in ready if p.kind_or_default == opts["--kind"]]
    if "--model" in opts:
        ready = [p for p in ready if p.model == opts["--model"]]
    if "--size" in opts:
        top = SIZES.index(opts["--size"])
        ready = [p for p in ready if p.size() in SIZES and SIZES.index(p.size()) <= top]
    ready.sort(key=lambda p: (p.kind_or_default != "bug", PRIOS.index(p.prio()), written(p, born)))
    picks = [(p, f"rework: {len(open_feedback(p))} play-test item(s) open") for p in rework]
    picks += [(p, f"{p.kind_or_default}, size {p.size()}, prio {p.prio()}, {p.model}, since {written(p, born)}")
              for p in ready]
    if not picks:
        print("nothing ready")
        return 1
    first, why = picks[0]
    print(f"Next: {first.slug} ({why})\n  {rel(first.path)}")
    if len(picks) > 1:
        print("Then:")
        for p, why in picks[1:4]:
            print(f"  {p.slug} ({why})")
    return 0


def board():
    index = plans()
    born = birthdays()
    week = git("log", "--since=7 days ago", "--diff-filter=A", "--name-only", "--format=", "--",
               "docs/plan/solved").split()
    out = [f"# Plan board\n\nWritten by `tools/plan.py board` on {date.today()}; not tracked by git.\n"]

    def col(title, rows):
        out.append(f"## {title} ({len(rows)})\n")
        out.extend(rows or ["-"])
        out.append("")

    implemented = [p for p in index.values() if p.stage == "implemented"]
    origin = BOARD
    col("Rework", [f"* {link_to(p, origin)}: " + "; ".join(open_feedback(p)) for p in implemented
                   if open_feedback(p)])
    col("Play-test", [f"* {link_to(p, origin)} ({days_since(waiting_since(p))} d): `{play_command(p) or '?'}`"
                      for p in sorted(implemented, key=waiting_since) if not open_feedback(p)])
    drafts = sorted((p for p in index.values() if p.stage == "draft"), key=lambda p: written(p, born))
    for kind in KINDS:
        col(f"Ready: {kind}", [f"* {link_to(p, origin)} size {p.size()}, {p.model}, prio {p.prio()}"
                               for p in drafts if p.kind_or_default == kind and draft_state(p, index) == "ready"])
    col("Blocked or on hold", [f"* {link_to(p, origin)}: {draft_state(p, index)}" for p in drafts
                               if draft_state(p, index) != "ready"])
    col("Solved in the last 7 days", [f"* {link_to(index[s], origin)}" for s in sorted({slug_of(f) for f in week})
                                      if s in index])
    BOARD.write_text("\n".join(out))
    print(f"wrote {rel(BOARD)}")
    return 0


TEMPLATES = {
    "bug": "## Repro\n\n1. \n\n## Expected\n\n\n## Actual\n\n\n## Fixed when\n\n"
           "* A test that names the plan fails before the fix and passes after it.\n\n" + CHECKLIST + "\n\n* \n",
    "feature": "## Idea\n\n\n## Plan\n\n* \n\n" + CHECKLIST + "\n\n* \n",
    "tooling": "## Problem\n\n\n## Plan\n\n* \n",
}


def new(args):
    if not args:
        print(__doc__, file=sys.stderr)
        return 2
    slug, rest = args[0], args[1:]
    opts = flags(rest, {"--kind": KINDS, "--model": MODELS, "--size": SIZES, "--prio": PRIOS}, positional=True)
    if opts is None:
        return 2
    title = " ".join(opts.pop("", []))
    missing = [f for f in ("--kind", "--model", "--size") if f not in opts]
    if missing or not title or not re.fullmatch(r"[a-z0-9-]+", slug):
        print(f"new SLUG --kind K --model M --size S [--prio P] TITLE (missing: {', '.join(missing) or 'title'})",
              file=sys.stderr)
        return 2
    if slug in plans():
        print(f"{slug} exists: {rel(plans()[slug].path)}", file=sys.stderr)
        return 1
    path = file_for(slug, "draft", opts["--kind"], opts["--model"])
    prio = f", prio {opts['--prio']}" if opts.get("--prio", "normal") != "normal" else ""
    path.write_text(f"# {title}\n\nStatus: draft {date.today()}, size {opts['--size']}{prio}, needs nothing first.\n\n"
                    + TEMPLATES[opts["--kind"]])
    print(f"wrote {rel(path)}")
    return 0


def flags(args, allowed, positional=False):
    """--flag value pairs (values checked against allowed[flag]); other words under "" when positional."""
    out = {}
    i = 0
    while i < len(args):
        a = args[i]
        if a in allowed:
            if i + 1 >= len(args) or args[i + 1] not in allowed[a]:
                print(f"{a}: one of {', '.join(allowed[a])}", file=sys.stderr)
                return None
            out[a] = args[i + 1]
            i += 2
        elif positional and not a.startswith("--"):
            out.setdefault("", []).append(a)
            i += 1
        else:
            print(f"unknown argument {a}", file=sys.stderr)
            return None
    return out


def move(slug, stage, force=False, note=""):
    index = plans()
    if slug not in index:
        print(f"no plan {slug}", file=sys.stderr)
        return 1
    plan = index[slug]
    stages = ("draft-sonet", "draft-opus", "implemented", "solved", "wont-do", "kind-bug", "kind-feature",
              "kind-tooling")
    if stage not in stages:
        print(f"stage: one of {', '.join(stages)}", file=sys.stderr)
        return 1
    if stage == "implemented" and plan.kind_or_default == "tooling":
        print("a tooling plan needs no play-test: solved")
        stage = "solved"
    if stage == "implemented" and not force:
        fails, _ = done_check(plan)
        if fails:
            print(f"not moved: fix the FAIL lines (or tools/plan.py move {slug} implemented --force)",
                  file=sys.stderr)
            return 1
    kind, model, where = plan.kind, plan.model, plan.stage
    if stage.startswith("kind-"):
        kind = stage[len("kind-"):]
    elif stage.startswith("draft-"):
        where, model = "draft", stage[len("draft-"):]
    else:
        where, model = stage, None
    old, new_path = plan.path, file_for(slug, where, kind, model)
    if old == new_path:
        print(f"{slug} is already {rel(new_path)}")
        return 0
    # Links inside the plan are relative to its folder: make them absolute before the move.
    text = old.read_text()
    text = REL_LINK.sub(lambda m: f"]({(old.parent / m.group(1)).resolve()}{m.group(2) or ''})", text)
    old.write_text(text)
    new_path.parent.mkdir(exist_ok=True)
    subprocess.run(["git", "mv", str(old), str(new_path)], cwd=ROOT, check=True)
    text = new_path.read_text()
    root = re.escape(str(ROOT))
    text = re.sub(rf"\]\(({root}/[^)#\s]+)(#[^)\s]*)?\)",
                  lambda m: f"]({os.path.relpath(m.group(1), new_path.parent)}{m.group(2) or ''})", text)
    new_path.write_text(text)
    index[slug] = Plan(slug, new_path, kind, model)
    fixed, broken = fix_links(index)
    print(f"{rel(old)} -> {rel(new_path)}, {fixed} links rewritten")
    today = date.today().isoformat()
    if where == "implemented" and plan.stage == "draft":
        if mark_stage(new_path, f"implemented {today}, waiting for the user's play-test", "implemented"):
            print("Status line: implemented today")
    if where == "solved":
        if mark_solved(new_path, today, note):
            print("Status line: solved today" + ("" if note else
                  " (add \"(play-tested by the user)\" when the user confirmed it: solve --playtested does)"))
    if where == "wont-do" and not re.search(r"won't do", status_line(new_path), re.I):
        print("Status line: add \"Won't do DATE: reason\" (docs/plans.md, Won't do)")
    return 1 if broken else 0


def scenarios_about(slug):
    """Scenarios whose header names the plan, and the others that only mention it."""
    about, mention = [], []
    for f in git("grep", "-l", "-w", "-e", slug, "--", "tests/scenarios/*.txt").split():
        if "/solved/" in f:
            continue
        first = next((line for line in (ROOT / f).read_text().splitlines() if line.startswith("#")), "")
        (about if re.search(rf"\bplans?\b[^.]*\b{re.escape(slug)}\b", first, re.I) else mention).append(f)
    return about, mention


def repath(old, new):
    """Rewrites mentions of the file path old to new in every kept markdown file and in solved plans of today."""
    pattern = re.compile(rf"(?<![\w/-]){re.escape(old)}")
    rewritten = 0
    for md in [*markdown_files(), *SOLVED.glob("*.md")]:
        text = md.read_text()
        if old not in text:
            continue
        new_text = pattern.sub(new, text)
        # Relative links: ../../tests/scenarios/x.txt
        new_text = re.sub(rf"(\.\./)+{re.escape(old)}", lambda m: m.group(0).replace(old, new), new_text)
        if new_text != text:
            md.write_text(new_text)
            rewritten += 1
    return rewritten


def solve(args):
    playtested = "--playtested" in args
    slugs = [a for a in args if a != "--playtested"]
    if not slugs:
        print("solve SLUG... [--playtested]", file=sys.stderr)
        return 2
    index = plans()
    unknown = [s for s in slugs if s not in index]
    if unknown:
        print(f"no plan {', '.join(unknown)}", file=sys.stderr)
        return 1
    code, parked, removed, behaviour = 0, [], [], []
    for slug in slugs:
        code |= move(slug, "solved", note=" (play-tested by the user)" if playtested else "")
        about, mention = scenarios_about(slug)
        for f in about:
            dest = f"tests/scenarios/solved/{Path(f).name}"
            (ROOT / dest).parent.mkdir(exist_ok=True)
            git("mv", f, dest, check=True)
            repath(f, dest)
            parked.append(Path(f).stem)
            if re.search(r"^\s*expect\b", (ROOT / dest).read_text(), re.M):
                behaviour.append(dest)
        for f in mention:
            print(f"  {f} mentions {slug} too: park it by hand if it is about it")
        script = play_script(slug)
        if script:
            git("rm", "-q", str(rel(script)), check=True)
            removed.append(script.name)
    for f in behaviour:
        print(f"  {f} has expect lines: its behaviour checks belong in a unit test (docs/plans.md, Solved)")
    msg = f"Solved: {', '.join(slugs)}" + (" (play-tested by the user)" if playtested else "") + "."
    if parked:
        msg += f" Scenarios to tests/scenarios/solved/: {', '.join(parked)}."
    if removed:
        msg += f" Play scripts removed: {', '.join(removed)}."
    print(f"\nCommit message:\n  {msg}")
    return code


def reject(slug, notes):
    index = plans()
    if slug not in index or index[slug].stage != "implemented":
        print(f"{slug}: not an implemented plan", file=sys.stderr)
        return 1
    path = index[slug].path
    lines = path.read_text().rstrip("\n").split("\n")
    i = next((k for k, line in enumerate(lines) if line.strip() == FEEDBACK), None)
    if i is None:
        lines += ["", FEEDBACK]
        i, end = len(lines) - 1, len(lines)
    else:
        end = next((k for k in range(i + 1, len(lines)) if lines[k].startswith("## ")), len(lines))
    rounds = sum(1 for line in lines[i:end] if line.startswith("### Round"))
    block = ["", f"### Round {rounds + 1} ({date.today()})", "", *(f"- [ ] {n}" for n in notes)]
    while end > i + 1 and not lines[end - 1].strip():
        end -= 1
    lines[end:end] = block + ([""] if end < len(lines) else [])
    path.write_text("\n".join(lines) + "\n")
    print(f"{rel(path)}: play-test round {rounds + 1}, {len(notes)} item(s). Fix them, tick them (- [x]), "
          f"then give the user the play command again (tools/plan.py playtest {slug}).")
    return 0


VERDICT = re.compile(r"^\s*(?P<v>✓|✔|ok|pass|\+|✗|✘|no|fail|-)\s*(?P<slug>[a-z0-9-]+)\s*(?::\s*(?P<note>.*))?$", re.I)


def parse_verdict(text):
    """{slug: (passed, [notes])} of a reply like "ok a; no b: too slow"."""
    out = {}
    for item in re.split(r"[\n;]", text):
        if not item.strip():
            continue
        m = VERDICT.match(item)
        if not m:
            raise ValueError(f"cannot read: {item.strip()!r} (\"ok SLUG\" or \"no SLUG: note\")")
        passed = m.group("v").lower() in ("✓", "✔", "ok", "pass", "+")
        old = out.get(m.group("slug"))
        if old and old[0] != passed:
            raise ValueError(f"{m.group('slug')}: both passed and failed")
        notes = (old[1] if old else []) + ([m.group("note").strip()] if m.group("note") else [])
        out[m.group("slug")] = (passed, notes)
    return out


def verdict(args):
    try:
        items = parse_verdict("\n".join(args))
    except ValueError as e:
        print(e, file=sys.stderr)
        return 2
    code = 0
    for slug, (passed, notes) in items.items():
        if not passed:
            code |= reject(slug, notes or ["failed the play-test (no detail given: ask the user)"])
    solved = [s for s, (passed, _) in items.items() if passed]
    if solved:
        code |= solve([*solved, "--playtested"])
    return code


def hold(slug, reason):
    p = need_draft(slug)
    if not p:
        return 1
    if on_hold(p):
        print(f"{slug} is on hold already")
        return 0
    edit_status(p.path, lambda s: f"{s} On hold: {reason.rstrip('.')}.")
    print(f"{slug}: on hold")
    return 0


def unhold(slug):
    p = need_draft(slug)
    if not p:
        return 1
    changed = edit_status(p.path, lambda s: " ".join(x for x in sentences(s) if not re.search(r"\bon hold\b", x,
                                                                                                re.I)))
    print(f"{slug}: " + ("off hold" if changed else "was not on hold"))
    return 0


def needs(slug, other, drop=False):
    index = plans()
    p = need_draft(slug)
    if not p or other not in index:
        print(f"no plan {other}" if p else "", file=sys.stderr)
        return 1
    link = link_to(index[other], p.path)
    if drop:
        def fn(s):
            s = re.sub(rf",?\s*\[{re.escape(other)}\]\([^)]*\)", "", s)
            s = re.sub(r"\bneeds\s+(and\s+)?first\b", "needs nothing first", s)
            return re.sub(r"\s*Needs first:\s*\.", "", s)
    else:
        def fn(s):
            if f"[{other}](" in s:
                return s
            if "needs nothing first" in s:
                return s.replace("needs nothing first", f"needs {link} first", 1)
            m = re.search(r"\bneeds (.*?) first\b", s)
            if m:
                return s[:m.end(1)] + f", {link}" + s[m.end(1):]
            return f"{s} Needs first: {link}."
    changed = edit_status(p.path, fn)
    print(f"{slug}: " + (("no longer needs " if drop else "needs ") + other if changed else "unchanged"))
    return 0


def need_draft(slug):
    p = plans().get(slug)
    if not p or p.stage != "draft":
        print(f"{slug}: not a draft", file=sys.stderr)
        return None
    return p


def lint():
    """Errors in the plans' names and Status lines (make check). Closed plans: their name only (history)."""
    errors = []
    seen = {}
    for f in plan_files():
        parsed = parse_name(f.name)
        where = rel(f)
        if not parsed:
            errors.append(f"{where}: not a plan name (SLUG[.KIND][.draft.MODEL].md)")
            continue
        slug, _, model = parsed
        if slug in seen:
            errors.append(f"{where}: slug {slug} also in {seen[slug]}")
        seen[slug] = where
        if f.parent in CLOSED and model:
            errors.append(f"{where}: a draft name in {f.parent.name}/ (moved by hand: tools/plan.py move)")
    index = plans()
    for p in index.values():
        if p.stage == "wont-do" and not re.search(r"won't do", p.head(), re.I):
            errors.append(f"{rel(p.path)}: no \"Won't do DATE: reason\" in its status")
        if not p.open:
            continue
        st = status_text(p.path)
        if st is None:
            errors.append(f"{rel(p.path)}: no Status line")
            continue
        if p.stage == "draft":
            if not re.match(r"draft\b", st, re.I):
                errors.append(f"{rel(p.path)}: a draft whose Status does not start with \"draft\"")
            if p.size() == "?":
                errors.append(f"{rel(p.path)}: no size in its Status (\"size S\", \"size M\" or \"size L\")")
            for b in blockers(p, index):
                if b.endswith("(won't do)"):
                    errors.append(f"{rel(p.path)}: needs {b}: rethink it")
        elif re.match(r"draft\b", st, re.I):
            errors.append(f"{rel(p.path)}: implemented, but its Status starts with \"draft\" "
                          f"(\"implemented DATE, waiting for the user's play-test. Was: ...\")")
    for e in errors:
        print(f"plan lint: {e}", file=sys.stderr)
    return 1 if errors else 0


def find(terms):
    """Plans that have every term (plan friction-intents): open ones first, then the newest solved."""
    words = [t.lower() for t in terms]
    order = {"implemented": 0, "draft": 1, "solved": 2, "wont-do": 3}
    hits = []
    for p in plans().values():
        text = p.text()
        hay = (p.path.name + "\n" + text).lower()
        if not all(w in hay for w in words):
            continue
        lines = text.splitlines()
        title = lines[0].lstrip("# ").strip() if lines else ""
        first = next((ln.strip() for ln in lines[1:] if any(w in ln.lower() for w in words)), "")
        hits.append((order[p.stage], -p.path.stat().st_mtime, p, title, first))
    for _, _, p, title, first in sorted(hits, key=lambda h: (h[0], h[1], h[2].slug)):
        print(f"{p.stage} {p.slug}: {title}")
        if first:
            print(f"    {first[:150]}")
    if not hits:
        print("no plan has " + " and ".join(terms))
        return 1
    return 0


def links():
    fixed, broken = fix_links(plans())
    if fixed:
        print(f"{fixed} links rewritten")
    return 1 if broken else 0


def main(argv):
    cmd, args = (argv[0], argv[1:]) if argv else ("", [])
    simple = {"status": status, "lint": lint, "links": links, "board": board}
    if cmd in simple and not args:
        return simple[cmd]()
    if cmd == "next":
        return next_plan(args)
    if cmd == "playtest" and len(args) <= 1:
        return playtest(*args)
    if cmd == "new":
        return new(args)
    if cmd == "find" and args:
        return find(args)
    if cmd == "move" and len(args) in (2, 3) and (len(args) == 2 or args[2] == "--force"):
        return move(args[0], args[1], force=len(args) == 3)
    if cmd == "solve":
        return solve(args)
    if cmd == "reject" and len(args) >= 2:
        return reject(args[0], args[1:])
    if cmd == "verdict" and args:
        return verdict(args)
    if cmd == "hold" and len(args) >= 2:
        return hold(args[0], " ".join(args[1:]))
    if cmd == "unhold" and len(args) == 1:
        return unhold(args[0])
    if cmd in ("needs", "unneed") and len(args) == 2:
        return needs(args[0], args[1], drop=cmd == "unneed")
    if cmd == "done-check" and len(args) == 1:
        p = plans().get(args[0])
        if not p:
            print(f"no plan {args[0]}", file=sys.stderr)
            return 1
        return 1 if done_check(p)[0] else 0
    print(__doc__, file=sys.stderr)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
