# Tools print with std::println

Status: solved 2026-11-15. Was: draft 2026-11-13, size M, needs [cpp23-standard](cpp23-standard.tooling.md) first.

Rank 5 of 5 in the C++23 set, the lowest gain: `-Wall` already checks `printf` format strings (`-Wformat`), so type
safety is mostly there, and `<print>` costs compile time. Do it last, or drop it.

## Problem

The tools print with `printf` (69 calls) and `fprintf` (29): `tools/balance/balance.cpp`, `tools/level/`,
`tools/model-viewer/viewer.cpp`. The game has 19 `printf` and 13 `fprintf`. `std::println` needs no length
modifiers (`%zu`, `%lld`), prints `std::string` and `std::string_view` without `.c_str()`, and has no trailing
`\n` to forget.

## Plan

* First measure on one file (`tools/balance/balance.cpp`): convert it, compare its compile time and the binary
  size before and after. More than 2x compile time for that file, or a tool binary more than 1 MB bigger: stop,
  write the numbers into this plan and ask the user to move it to won't do.
* Then the other tools: `printf("...\n", ...)` → `std::println("...", ...)`, `fprintf(stderr, ...)` →
  `std::println(stderr, ...)`, no newline → `std::print`. Translate the format specs: `%d` → `{}`, `%5.2f` →
  `{:5.2f}`, `%-12s` → `{:<12}`, `%5d` → `{:5}` (numbers align right, strings left: the same as `printf`), `%%` →
  `%`, literal `{` `}` → `{{` `}}`.
* The output must stay byte-identical where a script or a test reads it (`tools/plan.py`, `make check`, the
  level checker's messages in unit tests): diff each tool's output on the same input before and after
  (`bin/levelcheck content/levels/lvl*`, the balance tool's tables).
* Not in scope: `snprintf` into a fixed buffer (139 calls; `std::format_to_n` is no shorter), the logger, and
  `src/` (the game) unless the measurement shows no cost: then the 32 calls there too.
* `docs/development.md`: "tools print with `std::println`" in the code style list.
* `make check`. No behaviour change, no play-test.

## Result

Measured on `tools/balance/balance.cpp` (g++ `-O3`, the makefile flags): compile 6.04 s before, 10.4 s after (1.7x,
under the 2x limit); `bin/balance` 1933592 -> 2039920 bytes (+104 KB), `bin/levelcheck` +104 KB, `bin/levelgen`
+118 KB, `bin/levelconvert` +139 KB (under the 1 MB limit). Converted: `tools/balance`, `tools/level` (3 files),
`tools/model-viewer`, `tools/editor` (every `printf` / `fprintf`; `snprintf` stays). `src/` not converted: 1.7x
compile time for the one file is a cost. Output of balance (today, curve, table, fight, home, errors, usage),
levelcheck (all levels, `--quiet`, `--map`, `--legend`, usage), levelgen and levelconvert is byte-identical to before.
