# Readable hieroglyph cartouches

Status: draft 2026-10-10, ready (needs nothing first). From the lore review, picked by the user.

## Idea

Wall cartouches and inscriptions the player can read: interact near one, the archaeologist translates it into the
journal ("Inscriptions" section). Atmosphere first, hints second.

* Wall decals (today: `decals`, `src/world/dungeon_decor.cpp`) get a readable kind: a cartouche or a short text
  panel, drawn with real hieroglyphs (texture from a public-domain sign list, e.g. Gardiner signs).
* Interact shows a status line and adds the translation to the journal.
* Texts: real ones where they fit (offering formula "an offering which the king gives", tomb curses, Book of the Dead
  lines), plus hints: a riddle's hint, the way to a secret, a warning before a boss.
* Humour in the archaeologist's comments: "Translation: 'Whoever reads this, beware.' Too late."
* Level format: a decal object with a text id (`docs/levels.md`); texts in a data file like the riddles, no rebuild.
* [stargate-chamber](stargate-chamber.draft.md) and [tomb-curse-triggers](tomb-curse-triggers.draft.md) can use them.

## Tests

* Unit: reading adds the entry once; save and load keep it.
* Scenario with a screenshot of a cartouche and its journal page.
