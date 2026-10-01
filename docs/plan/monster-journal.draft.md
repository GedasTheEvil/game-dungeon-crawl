# Monster journal

Status: draft 2026-10-01.

## Idea

A full-screen bestiary next to the map and the inventory. The player learns about a monster type by fighting it:
entries start empty and fill in as the player kills and hits monsters. It is how the player finds out the
resistances from [damage-types-and-resistances.draft.md](damage-types-and-resistances.draft.md) without a wiki.

## Look

An explorer's field journal: notes in the shared papyrus look ([docs/ui.md](../ui.md)), written as handwritten
notes, not a stats table. A 1920s archaeologist with a notebook and pencil fits the Egyptian tomb theme.

* One page per monster type, a list or tabs of the seen types. Unseen types are not listed (or show "?").
* A sketch of the monster: the model rendered flat and tinted like ink or pencil, or a hand-drawn texture per type.
* Facts as short notes: "Tough shell, the sword glances off", "Club cracks it", "~40 HP".
* Unknown facts are blank or "?", so the player can see what is still left to learn.

## Unlocking

Proposed, numbers open:

| Trigger | Reveals |
|---|---|
| First seen | Sketch (blurred or outline only), "?" name |
| 1st kill | Name, description, HP, XP |
| 3 kills | Damage, speed, special moves (leap, summon, heal) |
| Hit with a damage type | That type's resistance: "weak", "normal" or "resists" |

Resistances unlock by hitting, not by kill count: hitting a bat with the club writes down "the club works". This
makes the player try other weapons, which is the goal of damage types. A kill count for resistances would only
reward grinding.

Optional: a status box toast when a new note is written ("Journal: scarab, resists slash").

## What

* Kill and hit counts per monster type and damage type, stored in the save game. Knowledge is per save game
  (decided 2026-10-01): a new game starts with an empty journal.
* Screen `src/ui/journal.cpp`, like `inventory.cpp` / `map_view.cpp`; a key and an entry in the in-game menu.
* Note texts per monster type next to `monster_kinds.cpp`.
* Scenario test: kill one monster, open the journal, screenshot.

## Related

* [damage-types-and-resistances.draft.md](damage-types-and-resistances.draft.md): the journal is where the
  resistances show; do it with or right after the damage types.
* [monster-strength.draft.md](monster-strength.draft.md): HP and stats shown in the journal follow its retune.
