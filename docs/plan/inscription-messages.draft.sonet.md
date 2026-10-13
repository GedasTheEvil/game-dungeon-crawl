# Short status messages for inscriptions; the comment only in the journal

Status: draft 2026-10-13, ready (needs nothing first). From the user. The "cannot read" message needs
[skill-hieroglyphs](skill-hieroglyphs.draft.sonet.md); whichever plan lands second adds it.

## Idea

Reading an inscription puts its title and the archaeologist's comment (the `N:` joke) on the status line today
(`Dungeon::readInscription`, `src/world/dungeon_interact.cpp`). The user (2026-10-13): the status line only says what
happened, the jokes belong in the journal only.

* Read (today every read; later: with the hieroglyph skill): "Deciphered the hieroglyphs. Journal updated."
* Not readable (without the skill, once [skill-hieroglyphs](skill-hieroglyphs.draft.sonet.md) is in): "Cannot
  understand this. Copied the hieroglyphs to the journal." This replaces that draft's "Hieroglyphs. If only I could
  read them."
* Seen again, nothing new in the journal (the user, 2026-10-13): "Already seen. In the journal." The same for a copy
  seen again without the skill.
* "The signs are too worn to read." (a text id with no text) stays.
* XP ([inscription-xp](inscription-xp.draft.sonet.md)): its "+N XP" goes on the deciphered line, e.g. "Deciphered
  the hieroglyphs. Journal updated. +N XP".

No title and no `N:` comment on the status line. The journal page keeps both, unchanged.

## How

* `Dungeon::readInscription`: the `Status` call prints the fixed lines above, chosen by `first`.
* Docs: `docs/inscriptions.md` (intro paragraph: the status line says the hieroglyphs were deciphered; the `title:`
  row: heading of the journal page only).

## Tests

* Unit (`tests/unit/inscriptions_test.cpp`, the "interact in front of an inscription" case): the first interact tells
  "Deciphered the hieroglyphs", and does not tell the title or the comment; the second tells "Already seen".
