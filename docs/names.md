# Royal names

A name gate (`!` in the [level legend](levels.md)) is a teleporter sealed by a royal name
([plan](plan/solved/secret-name-gates.md)). Interacting with it opens the riddle scroll: "Speak the name written in the
cartouche." The right name opens it for good: from then on it is a plain teleporter, both ways, to the secret room behind
it. Wrong names can be retried; there is no hint and no XP for the name itself. Esc walks away and the gate stays
sealed. Until it is opened, its plasma breathes into transparency and back.

The name is written in a name cartouche (`:`, an inscription with the text id 100) elsewhere on the same level. Read with
the hieroglyph skill (Words of Thoth), the journal page says "The name of the king: KHUFU". Without the skill the signs
are copied like any other inscription, and the gate's scroll, after a wrong name, says "A name is asked. The wall glyphs
I copied might hold it." Learning the skill later translates the copy, name and all.

Each run picks the names anew: New Game rolls a seed (`RunStats::nameSeed`, kept in the save game) that shuffles the
name list, level 1 the first name. No name repeats in a run as long as the list has at least 35 names (one per campaign level) (the unit tests
check it). The cartouche spells the name in signs (its `G:` line): a player who reads hieroglyphs can open the gate
without the skill.

The game loads every `content/names/*.txt` file at start-up. Add a name or edit one; no rebuild is needed. A sign no
`G:` line used before needs the hieroglyph sheet rebuilt (below).

## File format

Blank lines and `#` comment lines are ignored. Each entry starts with its `name:` line.

```
name: Ramesses
A: ramesses
A: ramses
N: Ramesses the Great. Built half of Egypt, signed the other half.
```

| Line | Meaning |
|---|---|
| `name: TEXT` | The name as the journal writes it, in capitals: "The name of the king: RAMESSES". |
| `title: TEXT` | Optional. Who bore it: "The name of the queen: NEFERTITI". Default: `king`. |
| `A: TEXT` | An accepted spelling, one line each. Compared like a riddle's answer ([riddles](riddles.md)): letters and digits only, lower case, so `Hor-Aha` matches `A: horaha`. At most 32 characters. |
| `G: CODES` | The name in signs by Gardiner code, as on its real cartouche (`Khufu`: `Aa1 G43 I9 G43`), up to 5: drawn in the name cartouche like an inscription's `G:` line ([inscriptions](inscriptions.md)). |
| `N: TEXT` | Optional. The archaeologist's comment, pencilled on the journal page before "Something on this level wants to hear it." Keep it to 70 characters: the page has two lines for both. |

A name without an `A:` line is skipped. Use plain ASCII: the fonts have no other characters.

## Checking a file

The game log (`game.log`) lists every problem as `file:line: message` and the count, for example
`Loaded 30 royal names`.

`levelcheck` warns about a name gate without a name cartouche or a cartouche without a gate, more than one of either on a
level, a cartouche next to its gate or out of reach while the gate is sealed, and an exit, key
or lever behind a name gate.

## The signs

`python3 tools/textures/inscriptions.py` bakes the hieroglyph sheet from the `G:` lines of the inscriptions and the names
([inscriptions](inscriptions.md)); a code the sheet lacks is a load error in `game.log`.
