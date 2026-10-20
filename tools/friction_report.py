#!/usr/bin/env python3
"""Counts what the checks and the rules cost agents, from the Claude Code transcripts (plan dev-friction-recheck).

    python3 -I tools/friction_report.py [--since YYYY-MM-DD] [--prompts FILE]

Reads the session transcripts of this project (main sessions and subagent threads) and prints counts only: check runs,
failures and run time, tidy checks that fired, fix loops, files read and edited, plan bookkeeping. --prompts writes the
user's typed prompts to FILE for a read by hand (keep that file out of the repo).
"""

import argparse
import collections
import json
import os
import re
import statistics
import sys
from datetime import datetime
from pathlib import Path

PROJECT = Path.home() / ".claude/projects/-home-gediminas-skucas-Apps-game-dungeon-crawl"
REPO = "/home/gediminas.skucas/Apps/game-dungeon-crawl/"

# Check commands, most specific first: a command counts as the first one it contains.
CHECKS = [
    ("make check", re.compile(r"\bmake\s+(-\S+\s+)*check\b")),
    ("make tidy", re.compile(r"\bmake\s+(-\S+\s+)*tidy(-all|-fix)?\b")),
    ("make format-check", re.compile(r"\bmake\s+(-\S+\s+)*format-check\b")),
    ("make test", re.compile(r"\bmake\s+(-\S+\s+)*test\b")),
    ("make unit", re.compile(r"\bmake\s+(-\S+\s+)*unit\b")),
    ("levelcheck", re.compile(r"bin/levelcheck\b")),
    ("make layers", re.compile(r"\bmake\s+(-\S+\s+)*layers\b")),
]
TIDY_WARNING = re.compile(r"^(\S+?):(\d+):\d+: (?:warning|error): .*\[([a-z0-9.,-]+)\]\s*$", re.M)
COMPILE_ERROR = re.compile(r"^\S+:\d+:\d+: error: (?!.*\[[a-z0-9.,-]+\]\s*$)", re.M)
FORMAT_FAIL = re.compile(r"not formatted|unstable", re.I)
UNIT_FAIL = re.compile(r"test cases:.*\|\s*[1-9]\d* failed|^ERROR: ", re.M)
LEVEL_WARN = re.compile(r"^\S*lvl\S*:.*warning", re.M | re.I)
EDIT_TOOLS = {"Edit", "Write", "MultiEdit", "NotebookEdit"}


def rel(path):
    return path[len(REPO):] if path.startswith(REPO) else path


def text_of(content):
    if isinstance(content, str):
        return content
    if isinstance(content, list):
        return "\n".join(b.get("text", "") for b in content if isinstance(b, dict))
    return ""


def when(stamp):
    return datetime.fromisoformat(stamp.replace("Z", "+00:00"))


def check_of(command):
    for name, pattern in CHECKS:
        if pattern.search(command):
            return name
    return None


def bash_files(command):
    """Files a shell command writes: sed -i targets, git mv, file redirections (rough)."""
    out = []
    if re.search(r"\bsed\s+(-\S*\s+)*-i", command):
        out += re.findall(r"((?:src|docs|tests|tools|content)/\S+\.\w+)", command)
    for m in re.finditer(r"\bgit mv\s+(\S+)\s+(\S+)", command):
        out += [m.group(1), m.group(2)]
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--since", default="2026-10-12")
    parser.add_argument("--prompts")
    args = parser.parse_args()
    since = datetime.fromisoformat(args.since + "T00:00:00+00:00")

    files = [p for p in PROJECT.rglob("*.jsonl") if datetime.fromtimestamp(p.stat().st_mtime).date() >= since.date()]
    runs = collections.defaultdict(lambda: {"n": 0, "fail": 0, "secs": [], "sessions": set()})
    tidy_fires = collections.Counter()  # check -> unique session:file:line
    tidy_sessions = collections.defaultdict(set)
    tidy_files = collections.Counter()
    compile_errors = 0
    loops = collections.defaultdict(list)  # check -> tool calls from a failed run to the next passing one
    edits = collections.Counter()
    edit_sessions = collections.Counter()
    read_only = collections.Counter()
    coedits = collections.Counter()
    tool_calls = 0
    bookkeeping = collections.Counter()
    prompts = []
    sessions = 0
    subagents = 0

    for path in files:
        pending = {}
        session_edits = set()
        session_reads = set()
        open_loop = {}  # check -> tool calls since it failed
        in_range = False
        for line in path.open():
            try:
                r = json.loads(line)
            except json.JSONDecodeError:
                continue
            stamp = r.get("timestamp")
            if not stamp or when(stamp) < since:
                continue
            in_range = True
            msg = r.get("message") or {}
            content = msg.get("content")
            if r.get("type") == "user" and r.get("origin", {}).get("kind") == "human" and not r.get("isMeta"):
                t = text_of(content).strip()
                if t and not t.startswith("<"):
                    prompts.append((stamp, path.stem[:8], t))
            if not isinstance(content, list):
                continue
            for b in content:
                if b.get("type") == "tool_use":
                    tool_calls += 1
                    for k in open_loop:
                        open_loop[k] += 1
                    name, inp = b.get("name"), b.get("input") or {}
                    pending[b["id"]] = (name, inp, when(stamp))
                    target = rel(inp.get("file_path", ""))
                    touched = []
                    if name in EDIT_TOOLS and target:
                        touched = [target]
                    elif name == "Bash":
                        touched = [rel(f) for f in bash_files(inp.get("command", ""))]
                        cmd = inp.get("command", "")
                        if re.search(r"git mv \S*docs/plan", cmd):
                            bookkeeping["plan git mv"] += 1
                        if re.search(r"grep.*docs/plan|docs/plan.*grep", cmd):
                            bookkeeping["plan link greps"] += 1
                    elif name == "Read" and target:
                        session_reads.add(target)
                    for t in touched:
                        edits[t] += 1
                        session_edits.add(t)
                        if t.startswith("docs/plan/"):
                            bookkeeping["plan file edits"] += 1
                        elif t == "docs/glossary.md":
                            bookkeeping["glossary edits"] += 1
                        elif t.startswith("tests/scenarios/"):
                            bookkeeping["scenario edits"] += 1
                        elif t.startswith("tests/unit/"):
                            bookkeeping["unit test edits"] += 1
                        elif t.startswith("docs/") or t == "AGENTS.md":
                            bookkeeping["other doc edits"] += 1
                        elif t.startswith("src/"):
                            bookkeeping["src edits"] += 1
                elif b.get("type") == "tool_result" and b.get("tool_use_id") in pending:
                    name, inp, start = pending.pop(b["tool_use_id"])
                    if name != "Bash":
                        continue
                    check = check_of(inp.get("command", ""))
                    if not check:
                        continue
                    out = text_of(b.get("content"))
                    run = runs[check]
                    run["n"] += 1
                    run["sessions"].add(path.stem)
                    run["secs"].append((when(stamp) - start).total_seconds())
                    warns = TIDY_WARNING.findall(out)
                    failed = bool(b.get("is_error")) or bool(warns) or bool(COMPILE_ERROR.search(out)) \
                        or (check in ("make check", "make format-check") and FORMAT_FAIL.search(out) is not None) \
                        or bool(UNIT_FAIL.search(out)) or (check == "levelcheck" and LEVEL_WARN.search(out) is not None)
                    compile_errors += len(COMPILE_ERROR.findall(out))
                    for f, ln, names in set(warns):
                        for n in names.split(","):
                            key = (path.stem, f, ln, n)
                            if key not in tidy_sessions[n]:
                                tidy_sessions[n].add(key)
                                tidy_fires[n] += 1
                                tidy_files[rel(f)] += 1
                    if failed:
                        run["fail"] += 1
                        open_loop.setdefault(check, 0)
                    elif check in open_loop:
                        loops[check].append(open_loop.pop(check))
        if not in_range:
            continue
        if "subagents" in path.parts:
            subagents += 1
        else:
            sessions += 1
        for f in session_edits:
            edit_sessions[f] += 1
        for f in session_reads - session_edits:
            read_only[f] += 1
        src = sorted(f for f in session_edits if f.startswith("src/"))
        for i, a in enumerate(src):
            for c in src[i + 1:]:
                coedits[(a, c)] += 1

    print(f"Since {args.since}: {sessions} sessions, {subagents} subagent threads, {tool_calls} tool calls\n")
    print("| Check | Runs (sessions) | Failed | Median s | p90 s | Total h | Fix loops: n, median calls |")
    print("|---|---|---|---|---|---|---|")
    for name, _ in CHECKS:
        r = runs.get(name)
        if not r:
            continue
        secs = sorted(r["secs"])
        p90 = secs[int(len(secs) * 0.9)] if secs else 0
        lp = loops.get(name, [])
        med = statistics.median(lp) if lp else 0
        print(f"| `{name}` | {r['n']} ({len(r['sessions'])}) | {r['fail']} | {statistics.median(secs):.0f} | {p90:.0f} "
              f"| {sum(secs) / 3600:.1f} | {len(lp)}, {med:.0f} |")
    print(f"\nCompile errors seen in check output: {compile_errors}")
    print("\nTidy checks (unique session:file:line), sessions:")
    for n, c in tidy_fires.most_common(25):
        print(f"  {n}: {c} ({len({k[0] for k in tidy_sessions[n]})})")
    print("\nTidy fires by file:", ", ".join(f"{f} {c}" for f, c in tidy_files.most_common(10)))
    print("\nBookkeeping (tool calls):", dict(bookkeeping))
    print("\nMost edited (sessions):", ", ".join(f"{f} {c}" for f, c in edit_sessions.most_common(15)))
    print("\nRead, not edited (sessions):", ", ".join(f"{f} {c}" for f, c in read_only.most_common(15)))
    print("\nCo-edited src pairs (sessions):", ", ".join(f"{a} + {b} {c}" for (a, b), c in coedits.most_common(10)))
    print(f"\nTyped user prompts: {len(prompts)}")
    if args.prompts:
        with open(args.prompts, "w") as out:
            for stamp, sid, t in sorted(prompts):
                out.write(f"=== {stamp[:16]} {sid}\n{t}\n\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
