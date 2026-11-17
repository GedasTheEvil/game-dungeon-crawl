# Read inscriptions lose their glint

Status: solved 2026-11-17 (play-tested by the user). Was: implemented 2026-11-17, waiting for the user's play-test. Was: draft 2026-11-17, size S, needs nothing first.

## Idea

From the user: an inscription already read loses its shine (the gold glint sweeping over it). The glint should mark
only walls with something new to read.

Today (plan [interact-prompt](interact-prompt.md), part 2): `Dungeon::InscriptionGlint`
(`src/sim/dungeon_interact.cpp`) gives 1 to an unread inscription, 0 to a translated one (read with the hieroglyph
skill) and 0.4 (`COPY_GLINT`) to a copy (read without the skill, plan skill-hieroglyphs). So a read-but-copied
inscription still shines, dimmer: that is what the user sees.

## Plan

* `InscriptionGlint`: any inscription in the journal (`FindInscription` finds it, copy or translated) returns 0.
  Drop `COPY_GLINT`. Only an inscription not in the journal yet keeps 1.
* Keep the glint off a too worn inscription (already 0) and keep the name cartouche (plan secret-name-gates) under the
  same rule.
* Update the comment on `InscriptionGlint` in `src/sim/dungeon.h` and the glint line in the glossary
  (`docs/glossary.md`, grep "glint") if it names the dimmer copy glint.
* Unit test (`tests/unit/interact_test.cpp`, `tests/levels/interact`): glint 1 before reading, 0 after reading a copy
  (no skill), 0 after reading with the skill. Replace the test that expects the copy's 0.4, if any.

## Play-test

* `bin/game --level 1` (inscriptions at col 19 / 20 / 25, row 35 to 42, near the start),  without the hieroglyph skill: the inscription shines; read it
  (Copy); the shine stops at once and stays off after leaving and coming back.
* The same with the skill (Read): the shine stops.
