# make check: measure the stage log a week later

Status: draft 2026-11-15, size S, needs nothing first.

## Problem

Point 3 of [check-speed-stage-breakdown](solved/check-speed-stage-breakdown.tooling.md): measure again a week after its tidy cache and stage log landed. Not before 2026-11-22, so the log holds a week of real runs.


## Plan

* `python3 -I tools/friction_report.py --since <the merge date>`: the per-stage table of `make check` (median, p90, share per stage, runs by changed files). Compare with the numbers in the solved plan.
* Compare the tidy stage: median and p90 against before (tidy median 178 s, p90 521 s; a gameplay_config.h change tidied 128 sources in 528 s).
* If a stage still dominates, write a draft for it; record the result in this plan.
 
