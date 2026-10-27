# Readable hieroglyph cartouches

Status: solved 2026-10-12 (play-tested by the user). From the lore review, picked by the user. Reference: [inscriptions.md](../../inscriptions.md).

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
* [stargate-chamber](stargate-chamber.md) and [trapped-chests](trapped-chests.md) (its tomb curses) can use them.

## Tests

* Unit: reading adds the entry once; save and load keep it.
* Scenario with a screenshot of a cartouche and its journal page.

## Implemented (2026-10-12)

* Tile type 15 `Inscription` (`i` in the legend), attribute = text id. Not a scattered decal: the level places it,
  so it gets an object of its own; the cell then has no prop, decal or torch. Drawn on the back wall from the atlas
  `textures/decorations/inscriptions.png`, built by `tools/textures/inscriptions.py` from the `G:` (Gardiner codes,
  Noto Sans Egyptian Hieroglyphs, OFL) and `style:` lines (cartouche, column, row, stela).
* Texts in `inscriptions/*.txt` (`InscriptionBook`, `src/world/inscriptions.*`), no rebuild. Interact: the title and
  the archaeologist's comment on the status line; the first time, the text (a copy) into the journal's new
  Inscriptions section (green ribbon "I"): the glyphs, the translation, the comment. Journal save version 6.
* Campaign: 32 inscriptions, one or two in each level 6 to 30 (the user: from 6 on, after the first heka; the
  hieroglyph skill comes later, [skill-hieroglyphs](skill-hieroglyphs.md)). Real texts (the offering formula, Meni's and
  Khentika's curses, the ushabti spell, the harper's song, Ptahhotep, Amenemhat, the negative confession, spell 30B,
  the Hymn to Hapi, a tomb robber's testimony, Ahmose's graffito) and hints: the vampire bat heals by biting (10),
  the lock order (12), loose ceilings (13), the scorpion queen's eggs (15), the dart plates (16, 23), the green key in
  the pillar (18), Apep burrowing (19, 20), the hidden treasure cistern (21), Sobek's charge (24), the hidden chamber
  (27), the tomb under the floor (29), the ankh behind Anubis (30).
* `levelcheck`: an inscription out of reach or without a text id; with the files, a text id with no text, one before
  level 6, the same id twice. Map view: a pencilled cartouche. Editor: icon and text-id hint.
* Tests: `tests/unit/inscriptions_test.cpp` (the file format, reading once, save and load, the campaign's
  inscriptions), scenario `tests/scenarios/solved/inscription.txt` (parked) (the wall, the status line, the journal page).
