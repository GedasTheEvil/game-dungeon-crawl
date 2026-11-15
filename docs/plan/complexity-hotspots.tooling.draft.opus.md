# Cognitive complexity report: the hardest functions as refactor candidates

Status: draft 2026-11-15, size M, needs nothing first.

## Problem

A controlled study of Claude Code on cleaner and messier copies of the same repositories (fewer static-analysis
violations, lower cognitive complexity; arXiv 2605.20049, 2026) found the same pass rate, but 7-8% fewer tokens and
34% fewer file revisits on the cleaner code. Formatting and lint style barely matter; hard-to-follow functions do. We
don't know which of our functions are the hard ones.

## Plan

* A report, not a gate: `make complexity` (or `tools/code_map.py complexity`) runs clang-tidy with only
  `readability-function-cognitive-complexity` (threshold about 25; pick it from the spread) over every source
  through the load gate, and prints the functions above it, most complex first: score, file:line, name, line count.
  Not in `.clang-tidy`, not in `make check`.
* Run it once, then cross the list with where agents spend effort: files read and edited most
  (`tools/friction_report.py`, the transcripts) and tidy's fires by file. A complex function nobody touches is not
  a candidate.
* Write the top 5-10 candidates into this plan (score, why it is hard: long switch, nested state, flags), each with
  the split proposed. One `tools/plan.py new` draft per refactor worth doing (kind tooling); behaviour must not change,
  the unit tests stay green.
* Docs: the command in docs/development.md (Code quality tools), one line.
