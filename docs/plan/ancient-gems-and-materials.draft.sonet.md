# Ancient gems and materials

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30.

## Problems

* **Ring gems** (`src/world/items.cpp` ring table, `content/texts/items.txt` `ring-*`): ruby and sapphire were
  unknown in pharaonic Egypt; emerald was mined only from Ptolemaic times; Greek "topaz" was really peridot, also late.
* **Amber gate**: `src/world/level.h` "Gold (Amber)", `content/inscriptions/campaign.txt`, readme. Baltic amber
  is almost unknown in Egypt.
* Ring metal tiers copper < silver < gold: in the Old and Middle Kingdom silver was rarer than gold (debatable;
  a game ladder, keep unless cheap).

## Plan

* Rename the gems, keeping effects and colours close:
  | Now | New | Effect |
  |---|---|---|
  | Ruby | Garnet (or carnelian) | regeneration |
  | Sapphire | Lapis lazuli ("night sky of Nut") | vigor |
  | Emerald | Green feldspar (or turquoise) | cleansing |
  | Topaz | Yellow jasper | speed |
  Rock crystal and amethyst stay. Rename the enum values (`RingGem`), effect texts, textures and docs. Saves store
  numbers, not names: check; old saves may be ignored (AGENTS.md).
* Gate colour "Amber" → "yellow jasper" in the visible texts (or "gold"; check it doesn't clash with a gold metal
  wording). The gem colour texture can stay. Watch for the turquoise gate vs a turquoise ring (pick feldspar then).
* Update `docs/glossary.md`.
