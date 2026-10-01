# Journal sections

Status: draft 2026-10-01.

## Idea

The [monster journal](monster-journal.draft.md) becomes the archaeologist's whole notebook, not only a bestiary.
One screen (one tab in [screen-switch-tabs.draft.md](screen-switch-tabs.draft.md)), split into sections with
bookmark ribbons or tabs on the page edge. Same rules as the monsters: everything written down comes from what the
player went through, and is kept per save game.

## Sections

| Section | Content | Filled when |
|---|---|---|
| Creatures | The monster journal as drafted. Bosses get their own page with the fight's notes. | seen, hit, killed |
| Riddles | Each riddle met: theme, question, and the answer if solved. | at a riddle gate |
| Traps | Spikes, death traps, rock falls: a sketch and a short note ("the ceiling cracks before it falls"). | first hit or first seen triggered |
| Expedition log | One line per level: name, monsters killed, treasure found / total, riddle solved or not, deaths. | on leaving a level |

### Riddles

* Solved: question and answer, ticked.
* Missed (walked away, Esc): decided 2026-10-01, the question stays with a "?" or an empty answer line, and the
  player can come back to it and answer it from the journal later. Gives a reason to think about it while playing,
  and the journal becomes useful, not only a record. A late answer gives one tenth of the gate's XP (decided
  2026-10-01). The gate gives 30% of the XP to the next level, at least 500 ([docs/riddles.md](../riddles.md)), so a
  late answer gives 3%, at least 50. The XP is fixed when the gate is met and saved with the riddle, so holding a
  riddle back until a level up gives nothing extra.
* The hint is written down only if it was shown.

### Potions

Skipped: the inventory already says what each potion does. Only worth a section if potions become unidentified
(unknown colour until drunk once), then it fits the same "learn by doing" rule.

## Later, if the game grows

* Lore notes: wall inscriptions or papyri found in the tomb, a story told in pieces.
* Map notes are already on the draft map; no journal section for them.

## Open

* Section order and how many fit before the page edge is crowded (4 looks right).
* Expedition log needs per-level stats saved in the save game.
