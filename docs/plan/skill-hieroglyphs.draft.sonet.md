# Skill: deep mystic knowledge (read hieroglyphs)

Status: draft 2026-10-10. Ready: [skills](solved/skills.md) is in, and so is
[hieroglyph-cartouches](solved/hieroglyph-cartouches.md) (solved 2026-10-12). From the
user: an easy first choice.

Where it hooks in: `Dungeon::readInscription` (`src/world/dungeon_interact.cpp`) reads every inscription today; gate
it on the skill there. `JournalInscription` (`src/world/journal.h`) has no "not yet read" flag yet: add one (journal
save version 7) for the entry kept before the skill, and fill it in on learning. Inscriptions stand in levels 6 to 30
(`INSCRIPTIONS_FROM_LEVEL`), after the first heka.

The skill is on the Heka screen already, sealed: implementing it clears its `sealed` in `SKILLS`
(`src/world/skills.cpp`) and checks its card text there.

## Idea

Without the skill, the player cannot read inscriptions: interact gives "Hieroglyphs. If only I could read them." and
the journal keeps a "not yet read" entry. With it, every inscription is read, also the ones met before (the journal
fills in).

* Name on the card: "Deep mystic knowledge" (user's), or "Words of Thoth" (implementer's choice).
* What it unlocks: the cartouche texts and their hints; optional extras, e.g. a riddle gate's hint shown at once, or
  secret doors marked by an inscription (the [Stargate chamber](stargate-chamber.draft.opus.md) hint).
* An easy first choice: cheap in power, rich in story. Without it nothing on the exit path is lost (checker rule:
  hints only, never required).

## Tests

* Unit: inscriptions read before learning are filled in after; reading needs the skill.
