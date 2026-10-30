# Royal name signs

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

Opus: hieroglyph data and maybe the cartouche drawing ([hieroglyph-cartouches](solved/hieroglyph-cartouches.md),
[egyptian-names](solved/egyptian-names.md)).

## Problems

* Early kings wrote their names in a serekh (palace façade with a falcon), not a cartouche: Narmer, Hor-Aha, Djer,
  Den, Anedjib, Semerkhet, Qaa, Merneith, Peribsen, Khasekhemwy, Djoser. Cartouches start around Huni / Sneferu.
  `docs/names.md` claims the signs are "as on its real cartouche".
* Wrong or missing signs (`content/names/kings.txt`):
  * Narmer is K4 (catfish) + U23 (chisel), not N35 D21.
  * Tutankhamun lacks Y5 (dropped by the 5-sign cap).
  * Merneith should use R24 for Neith.
  * Anedjib starts with D36 (ꜥ), not M17.

## Plan

* Fix the signs (add the glyphs to the font / sheet if missing).
* Either draw a serekh frame for the early kings, or drop them from the cartouche name pool and say so in
  `docs/names.md`. Implementer's choice; the serekh is nicer but needs a new frame drawing.
* Raise the sign cap if Tutankhamun's full name needs it, or note the shortening.
