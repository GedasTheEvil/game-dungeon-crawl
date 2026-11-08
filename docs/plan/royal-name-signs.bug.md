# Royal name signs

Status: implemented 2026-11-08, waiting for the user's play-test. Was: draft 2026-10-30, size M, needs nothing first. From the project audit of 2026-10-30.

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

## Done

* Signs fixed (`content/names/kings.txt`): Narmer K8 U23 (K8 is the catfish with barbels in the Noto font; K4 is
  the oxyrhynchus fish), Anedjib V26 F34 (his serekh's Adjib, as on Wikipedia), Merneith U6 R24, Tutankhamun with Y5,
  and the other early Horus names as on their serekhs: Hor-Aha D34, Djer M37, Semerkhet S29 U23 F32, Qaa N29 D36,
  Peribsen O1 F34 S29 N35, Khasekhemwy N28 S42 S42, Djoser's Horus name Netjerikhet R8 D21 F32 (`A: netjerikhet`).
* A serekh frame: a name's `frame:` line (`serekh`, `seth-serekh` for Peribsen, `twin-serekh` for Khasekhemwy, or any
  inscription style). Three new plates (`tools/textures/inscriptions.py`, plate grid 3 x 3), the name box takes 4
  signs. Merneith, a queen with no Horus name, on her tomb stela. The journal page keeps the frame (save version 15),
  the gate's scroll asks for "the name written in the serekh".
* Cartouche cap 5 -> 6: Tutankhamun's full amn-tut-ankh fits.
* The `names PATH` scenario / play command (one name file: every level has it).

## Play-test

`bin/game --play tests/play/royal-name-signs.txt`

* Walk right to the cartouche: Narmer's catfish and chisel in a serekh, the falcon on top, the palace niches below.
* Read it, open the journal (J): the page shows the same serekh.
* Walk on to the gate, interact: the scroll asks for "the name written in the serekh".
* Edit the `names` line of the script to peribsen / khasekhemwy / merneith / tutankhamun: Seth on top, both animals
  facing each other, a stela, six signs in the cartouche.
