# Ancient gems and materials

Status: solved 2026-11-01 (play-tested by the user). Was: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30. Implemented 2026-11-01: ruby,
sapphire, emerald, topaz are garnet, lapis (lazuli), (green) feldspar, (yellow) jasper (`RingGem`, `ItemKind`,
`RingEffect`, the lore and field note keys and texts); colours and effects unchanged. The gold gate's gem is "Gold"
(not jasper: the ring has it), the Book of Gates inscription says "the golden door". Lapis is now both the blue gate's
gem and a ring gem (no other blue stone of the time). Metal ladder kept. Play-test:
`bin/game --play tests/play/ring-accessories.txt`.

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
