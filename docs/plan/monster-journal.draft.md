# Monster journal

Status: draft 2026-10-01. Done 2026-10-01, verified in play; the resistances and the toast (only for resistance
notes) followed with [damage-types-and-resistances.md](damage-types-and-resistances.md), to verify in play. Data: `Journal::Creatures()` (`src/world/journal.h`), notes: `MonsterKind::note`
(`src/world/monster_kinds.cpp`), page: `JournalScreen::DrawCreature` (`src/ui/journal_view.cpp`).

## Idea

A full-screen bestiary next to the map and the inventory. The player learns about a monster type by fighting it:
entries start empty and fill in as the player kills and hits monsters. It is how the player finds out the
resistances from [damage-types-and-resistances.md](damage-types-and-resistances.md) without a wiki.

## Look

An explorer's field journal: notes in the shared papyrus look ([docs/ui.md](../ui.md)), written as handwritten
notes, not a stats table. A 1920s archaeologist with a notebook and pencil fits the Egyptian tomb theme.

* One page per monster type, a list or tabs of the seen types. Unseen types are not listed (or show "?").
* A sketch of the monster: decided 2026-10-01, the 3D model rendered flat and ink-tinted, in the same colour as the
  journal's handwriting ([handwritten-journal-font.md](solved/handwritten-journal-font.md)).
* Facts as short notes: "Tough shell, the sword glances off", "Club cracks it", "~40 HP".
* Unknown facts are blank or "?", so the player can see what is still left to learn.

## Unlocking

Decided 2026-10-01: every note comes from what the archaeologist went through, never from a kill count. No
grinding; each fact is learnt the way it would be in real life.

| Trigger | Reveals |
|---|---|
| First seen | Sketch (blurred or outline only), "?" name, where it was found (level number / name) |
| 1st kill | Name, description, HP |
| Hit with a damage type | That type's resistance: "weak", "normal" or "resists" |
| Hit by it | How hard it hits (its damage) |
| Sees it do a special move | That move (leap, summon, heal, climbs out of its coffin) |

* Hitting a bat with the club writes down "the club works", so the player tries other weapons, which is the goal of
  damage types.
* The damage only shows once the monster has hit the player: kill it with the bow from afar and its bite stays
  unknown.
* No XP in the journal.
* "Found on": first level seen, maybe all levels seen.

Optional: a status box toast when a new note is written ("Journal: scarab, resists slash").

## What

* Per monster type, flags stored in the save game: seen, killed, hit by it, each damage type tried, each special
  move seen, levels found on. Knowledge is per save game
  (decided 2026-10-01): a new game starts with an empty journal.
* Screen `src/ui/journal.cpp`, like `inventory.cpp` / `map_view.cpp`; a key and an entry in the in-game menu.
* Note texts per monster type next to `monster_kinds.cpp`.
* Scenario test: kill one monster, open the journal, screenshot.

## Related

* [damage-types-and-resistances.md](damage-types-and-resistances.md): the journal is where the
  resistances show; do it with or right after the damage types.
* [monster-strength.draft.md](monster-strength.draft.md): HP and damage shown in the journal follow its retune.
* [journal-sections.draft.md](journal-sections.draft.md): the journal as a whole notebook (creatures, riddles,
  traps, expedition log).
