# Riddles

A riddle gate (`?` in the [level legend](levels.md)) asks a riddle when the player interacts with it. A right answer
gives 30% of the XP from the current level to the next, at least 500 XP. Wrong answers can be retried; after two
misses the scroll shows the hint. Esc walks away without the reward, and the gate is spent either way.

Every riddle met is written down in the journal (J, Riddles ribbon), with the hint once it was shown. A riddle
walked away from can be answered from there later, for a tenth of the gate's reward (at least 50 XP), fixed when the
gate was met. The save game keeps a copy of each riddle, so editing a riddle file does not change the journal.

The game loads every `riddles/*.txt` file at start-up. Add a file or edit one; no rebuild is needed.
Riddles are dealt from a shuffled deck, so none repeats until all of them were asked.

## File format

One theme per file. Blank lines separate riddles. `#` starts a comment line.

```
theme: Palindrome

Q: A man, a plan, a canal: ______
A: panama
H: It reads the same backwards.
```

| Line | Meaning |
|---|---|
| `theme: NAME` | Heading on the scroll for the riddles below it. Default: the file name. |
| `Q: TEXT` | Question. Several `Q:` lines start new lines on the scroll; long lines wrap. About 6 lines fit. |
| `A: TEXT` | Accepted answer. Add one `A:` line per variant (`42`, `forty two`). At most 32 characters. |
| `H: TEXT` | Optional hint, shown after two wrong answers. Without it, the hint gives the answer length. |

Answers are compared on letters and digits only, lower case, with a leading `a`, `an` or `the` dropped:
`The Force!`, `force` and `FORCE` all match `A: force`.

Use plain ASCII: the fonts have no other characters (curly quotes and dashes show as `?`).

## Checking a file

The game log (`game.log`) lists every problem as `file:line: message` and the riddle count, for example
`Loaded 79 riddles from 4 files in riddles`. A riddle without an `A:` line is skipped.

To see a file on screen, point a scenario at it with `riddles FILE` (see [testing.md](testing.md)), as
`tests/scenarios/riddle.txt` does.
