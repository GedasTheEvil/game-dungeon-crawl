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
name list, level 1 the first name. No name repeats in a run as long as the list has at least 30 names (the unit tests
check it). The seed also picks each level's cartouche picture, one of the 16 sign groups in
`media/textures/decorations/name_cartouches.png`. The pictures do not spell the names: the skill translates them.

The game loads every `content/names/*.txt` file at start-up. Add a name or edit one; no rebuild is needed, nothing to
bake.

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
| `N: TEXT` | Optional. The archaeologist's comment, pencilled on the journal page before "Something on this level wants to hear it." Keep it to 70 characters: the page has two lines for both. |

A name without an `A:` line is skipped. Use plain ASCII: the fonts have no other characters.

## Checking a file

The game log (`game.log`) lists every problem as `file:line: message` and the count, for example
`Loaded 30 royal names`.

## The pictures

`python3 tools/textures/inscriptions.py` bakes the cartouche pictures with the inscription atlas: their sign groups are
`NAME_CARTOUCHES` in the tool, by Gardiner code as an inscription's `G:` line ([inscriptions](inscriptions.md)).
