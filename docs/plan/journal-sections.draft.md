# Journal sections

Status: draft 2026-10-01, refined 2026-10-01 (sections, book look, stages).

## Idea

The [monster journal](monster-journal.draft.md) becomes the archaeologist's whole notebook, not only a bestiary.
One screen (one tab in [screen-switch-tabs.draft.md](screen-switch-tabs.draft.md)), split into sections. Same rules
as the monsters: everything written down comes from what the player went through, and is kept per save game.

## Look: a book with ribbon bookmarks

Decided 2026-10-01: the journal is a book, open as a two-page spread. Coloured ribbon bookmarks hang from the
right edge, one per section; a click on a ribbon opens that section's first page.

Proposals, to confirm when the work starts:

* Spread on the 160 x 100 canvas in the shared look ([docs/ui.md](../ui.md)): left and right page, a spine in the
  middle. Creatures: sketch on the left, notes on the right. Riddles and field notes: text across both pages.
* Ribbons: one colour per section (creatures red, riddles lapis, field notes ochre?). The open section's ribbon
  sticks out further. Hover shows the section name.
* Page turn: arrow keys / mouse wheel, page corner arrows to click, page number at the bottom.
* Decided 2026-10-01: key J, an entry in the in-game menu; the game pauses while it is open, like the inventory
  and map. It opens on the last section and page looked at (creatures the first time).
* Empty sections (nothing learnt yet) keep their ribbon; the page says "Nothing written yet".
* Notes in the archaeologist's handwriting, not papyrus: [handwritten-journal-font.draft.md](handwritten-journal-font.draft.md),
  after stage 1. Until then the notes use papyrus.

## Sections

Decided 2026-10-01, in this order:

| Section | Content | Filled when |
|---|---|---|
| Creatures | The monster journal as drafted. Bosses get their own page with the fight's notes. | seen, hit, killed |
| Riddles | Each riddle met: theme, question, and the answer if solved. | at a riddle gate |
| Field notes | Short help on the game's rules: HP, XP and levels, stamina, potions, ... | see below |

### Riddles

* Solved: question and answer, ticked.
* Missed (walked away, Esc): decided 2026-10-01, the question stays with a "?" or an empty answer line, and the
  player can come back to it and answer it from the journal later. Gives a reason to think about it while playing,
  and the journal becomes useful, not only a record. A late answer gives one tenth of the gate's XP (decided
  2026-10-01). The gate gives 30% of the XP to the next level, at least 500 ([docs/riddles.md](../riddles.md)), so a
  late answer gives 3%, at least 50. The XP is fixed when the gate is met and saved with the riddle, so holding a
  riddle back until a level up gives nothing extra.
* The hint is written down only if it was shown.

Decided 2026-10-01:

* The save keeps a copy of each riddle met (theme, question, answers, hint shown or not, solved, the late XP), not
  an index into `riddles/*.txt`: those files can change between saves.
* A late answer opens the riddle scroll over the journal: same typing, same hint after two misses, unlimited
  retries.
* A riddle met again at another gate keeps one journal entry; a right answer at the gate gives the gate's full XP.
* One riddle per page, two per spread.
* Old saves have no journal data: the journal starts empty, no migration.

### Field notes

Idea only, refined later. Small descriptions in the archaeologist's voice of what the HUD numbers mean and how
they work: hit points, XP and levelling, stamina (running, jumping), potions, weapon grades, keys and gates.

Decided 2026-10-01: each note is written the first time it matters (first level up writes the XP note, first time
out of stamina the stamina note), like the rest of the journal.

### Potions

No section of their own: the inventory already says what each potion does; the field notes cover them in general.
Only worth more if potions become unidentified (unknown colour until drunk once).

## Stages

The creatures section does not have to wait for damage types: their resistance notes can come later.

1. Book screen: spread, ribbons, page turn, its key (J?); the screen tab comes with the tabs plan. Riddles section (needs riddle state in the save game).
2. Field notes.
3. Creatures without resistances ([monster-journal.draft.md](monster-journal.draft.md)): seen, kill, HP, its hit,
   special moves.
4. Resistance notes, with or after [damage-types-and-resistances.draft.md](damage-types-and-resistances.draft.md).

Order with the other plans (2026-10-01): stage 1, then the [handwritten font](handwritten-journal-font.draft.md), then
[screen-switch-tabs.draft.md](screen-switch-tabs.draft.md), then stages 2-4.

## Later, if the game grows

* Traps: spikes, death traps, rock falls, a sketch and a short note ("the ceiling cracks before it falls").
* Expedition log: one line per level (name, monsters killed, treasure found / total, riddle, deaths). Needs
  per-level stats in the save game.
* Lore notes: wall inscriptions or papyri found in the tomb, a story told in pieces.
* Map notes are already on the draft map; no journal section for them.

## Open

* Sketch of a monster: the model rendered flat and ink-tinted, or a drawn texture per type
  ([monster-journal.draft.md](monster-journal.draft.md)). Needed for stage 3 only.
