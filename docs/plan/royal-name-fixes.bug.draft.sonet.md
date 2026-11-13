# Royal names: two misspelled cartouches, two wrong notes

Status: draft 2026-11-13, size S, needs nothing first.

From the lore audit of 2026-11-13. `content/names/kings.txt` (format: [names.md](../names.md)): the `G:` line spells
the name in signs "as on its monuments", and a player who reads hieroglyphs can open the name gate from the signs.
Two spellings miss a sound; two `N:` notes state a wrong fact. The other 56 entries were checked and are fine.
Every sign proposed below is on the hieroglyph sheet already (`media/textures/decorations/hieroglyphs.txt`: G17,
G39, C7), so no atlas rebuild is needed.

## Repro

1. `bin/game --level N` on a level whose name roll is Ptolemy or Siptah (the names are shuffled per run by
   `RunStats::nameSeed`; a unit test or `tools/` lookup of the `G:` line is quicker), read the name cartouche.
2. Read the journal page of Amenemhat's or Den's cartouche.

## Expected

The signs spell the name; the notes are true.

## Actual

1. **Ptolemy** (`kings.txt:228`): `G: Q3 X1 G43 E23 S29` reads p-t-w-l-s. The cartouche (Rosetta Stone, the one
   Champollion read first) is p-t-w-l-m-y-s: Q3 X1 G43 E23, then the m (Aa15 or G17), M17 M17, S29. The m is a
   sound of the name, not a filler. Sure.
2. **Siptah** (`kings.txt:325`): `G: S29 Q3 X1 V28` reads s-ptH. The name is sA-ptH, "son of Ptah": the sA is the
   pintail duck G39 (or Aa17), not the folded cloth S29 (s). Fairly sure (his cartouches write G39).
3. **Amenemhat** (`kings.txt:193`): "Built a labyrinth, says Herodotus." Herodotus (Histories 2.148) describes the
   labyrinth by Lake Moeris but credits it to the twelve kings of the Dodecarchy, not to Amenemhat. Manetho credits
   it to Lamares/Lachares, i.e. Amenemhat III (Hawara). Sure.
4. **Den** (`kings.txt:173`): "Three letters. The shortest name on the list." Qaa (`kings.txt:247`), added later by
   [denser-levels](solved/denser-levels.md), has three letters too. Internal.

Optional, lower confidence: **Setnakhte** (`kings.txt:337`) `G: S29 X1 N35 Aa1 X1` reads st-nxt; the god's name is
stX, and his cartouches write it with the Seth animal. `C7 N35 Aa1 X1` (Seth + nxt) is closer. Verify against a
cartouche before changing it.

## Fix

1. Ptolemy: `G: Q3 X1 G43 E23 G17 S29` (6 signs, the cartouche's cap; the y dropped, as `Cleopatra` drops signs).
2. Siptah: `G: G39 Q3 X1 V28`.
3. Amenemhat note (70 characters at most, [names.md](../names.md)): e.g. "Built a labyrinth, says Manetho.
   Herodotus got lost in it." (check the length), or "His labyrinth amazed Herodotus. Nobody found the way out
   since."
4. Den note: e.g. "Three letters, and the first king shown in the double crown." (his ivory labels; sure).
5. Setnakhte only if verified.

## Fixed when

* A test that names the plan fails before the fix and passes after it: e.g. in the royal-names unit test, Ptolemy's
  glyphs contain G17 (or Aa15), Siptah's start with G39, Den's note has no "shortest".

## Play-test

* In a run, read a name cartouche of Ptolemy or Siptah (or check the journal page after a `give`/`level` setup that
  rolls one): the signs match the corrected `G:` line, and the gate still opens to the name.
