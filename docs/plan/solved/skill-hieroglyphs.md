# Skill: deep mystic knowledge (read hieroglyphs)

Status: solved 2026-10-13 (play-tested by the user). Done: the skill gate, the
journal flag (save version 7), the copies translated on learning, the card unsealed. Not done:
[inscription-xp](inscription-xp.md) is not in, so no XP part. Status lines as in
[inscription-messages](inscription-messages.md). The extras (riddle hints, secret doors) are left out.

Where it hooks in: `Dungeon::readInscription` (`src/world/dungeon_interact.cpp`) reads every inscription today; gate
it on the skill there. `JournalInscription` (`src/world/journal.h`) has no "not yet read" flag yet: add one (journal
save version 7) for the entry kept before the skill, and fill it in on learning. Inscriptions stand in levels 6 to 30
(`INSCRIPTIONS_FROM_LEVEL`), after the first heka.

The skill is on the Heka screen already, sealed: implementing it clears its `sealed` in `SKILLS`
(`src/world/skills.cpp`) and checks its card text there.

## Idea

Without the skill, the player cannot read inscriptions: interact gives "Cannot understand this. Copied the
hieroglyphs to the journal." ([inscription-messages](inscription-messages.md); the pencilled joke goes
in the journal only) and
the archaeologist copies the signs into the journal anyway (the user, 2026-10-12): an Inscriptions page with the
glyphs from the atlas, a "?" or "Inscription" heading instead of the title, no translation, and a pencilled line
like "No idea what it says. Copied it anyway." With the skill, every inscription is read, also the ones copied before:
learning it fills in their title, translation and comment (the next journal open shows them; a toast or status
"N inscriptions translated" is a nice touch).

XP ([inscription-xp](inscription-xp.md)): given when an inscription is translated, not when it is copied.
The copies filled in on learning give their XP then, once each.

* Name on the card: "Deep mystic knowledge" (user's), or "Words of Thoth" (implementer's choice).
* What it unlocks: the cartouche texts and their hints; optional extras, e.g. a riddle gate's hint shown at once, or
  secret doors marked by an inscription (the [Stargate chamber](../stargate-chamber.draft.opus.md) hint).
* An easy first choice: cheap in power, rich in story. Without it nothing on the exit path is lost (checker rule:
  hints only, never required).

Copying needs no skill, so inscriptions can stand from level 1 on: [more-inscriptions](../denser-levels.draft.opus.md#more-inscriptions-from-level-1-on)
places them, after this plan.

## Tests

* Unit: without the skill, interact copies the entry (glyphs, no text) once; learning fills in the copies; reading
  with the skill writes the full entry; the XP comes with the translation, once.
