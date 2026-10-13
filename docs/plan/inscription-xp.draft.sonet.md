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
* Status line: the third line becomes "(Translation written in the journal, +N XP)". If
  [inscription-messages](inscription-messages.draft.sonet.md) is in: "Deciphered the hieroglyphs. Journal updated.
  +N XP".
* Docs: `docs/inscriptions.md` (the first read gives XP), `docs/riddles.md` need not change.

## Tests

* Unit (`tests/unit/inscriptions_test.cpp`, the SimWorld case): the first interact raises `player.stats` XP by
  `riddleXP(level) / 3`, the second does not; the status line says "+N XP".

## With the hieroglyph skill

Decided by the user (2026-10-12): without [skill-hieroglyphs](skill-hieroglyphs.draft.sonet.md) the archaeologist only
copies the signs. The XP comes with the translation: when read with the skill, or, for the copies made before, when
learning the skill fills them in (once each, at the level then). If this plan lands first, every first read gives
the XP (all are read today); skill-hieroglyphs then moves it to the translation.
