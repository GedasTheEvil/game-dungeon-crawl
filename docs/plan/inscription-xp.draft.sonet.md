# XP for reading an inscription

Status: draft 2026-10-12, ready (needs nothing first). From the user, after
[hieroglyph-cartouches](solved/hieroglyph-cartouches.md) was solved.

## Idea

Reading a wall inscription for the first time gives XP: a third of a riddle's reward. Reading it again gives
nothing, like the journal entry (`Journal::ReadInscription` returns true only the first time).

## How

* Amount: `riddleXP(level) / 3` (`src/world/progression.h`), the player's level when reading, like a riddle gate's
  reward (`PlayerStats::RiddleXP`). Riddles give at least 500, so an inscription gives at least 166. A named constant
  for the divisor in `src/core/gameplay_config.h` (`INSCRIPTION_XP_PART` or similar), next to the riddle's numbers.
* Where: `Dungeon::readInscription` (`src/world/dungeon_interact.cpp`), when `first` is true:
  `sim.player->stats.AddXP(xp, *sim.events)`. A level up shows its own status line through `AddXP`.
* Status line ([inscription-messages](solved/inscription-messages.md)): "Deciphered the hieroglyphs. Journal updated.
  +N XP".
* Docs: `docs/inscriptions.md` (the first read gives XP), `docs/riddles.md` need not change.

## Tests

* Unit (`tests/unit/inscriptions_test.cpp`, the SimWorld case): the first interact raises `player.stats` XP by
  `riddleXP(level) / 3`, the second does not; the status line says "+N XP".

## With the hieroglyph skill

Decided by the user (2026-10-12): without [skill-hieroglyphs](solved/skill-hieroglyphs.md) the archaeologist only
copies the signs. The XP comes with the translation: when read with the skill, or, for the copies made before, when
learning the skill fills them in (once each, at the level then). skill-hieroglyphs is in, so implement both cases:
the gate in `Dungeon::readInscription`, the copies in `Journal::TranslateInscriptions` (called from
`src/ui/skills_view.cpp` and the `learn` command in `src/test/scenario.cpp`).

No penalty for learning the skill late (the user, 2026-10-13): a copy translated later gives the full XP, the same
`riddleXP(level) / 3` as reading it with the skill, at the player's level then (`riddleXP` grows with the level, so
never less). The toast on learning adds the total, e.g. "2 inscriptions translated, +N XP".
