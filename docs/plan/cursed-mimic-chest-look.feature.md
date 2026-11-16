# Cursed mimic: the chest it leaves keeps its cursed look

Status: implemented 2026-11-16, waiting for the user's play-test. Was: draft 2026-11-16, size M, needs nothing first.

## Idea

A killed cursed mimic turns into a treasure chest (`Dungeon::updateMimicChest` → `dropChest` → `placeDroppedChest`,
`src/sim/dungeon_monsters.cpp`, `src/sim/dungeon_interact.cpp`). That chest is drawn as the normal treasure chest. The
user wants it to look like the cursed mimic it was: the cursed mimic's texture (`monsters/mimic_cursed`), not the plain
chest's.

## Plan

* The dropped chest remembers where it came from: a cursed look for the chest a cursed mimic leaves (a new
  `Container` kind, a look flag on the dropped `ContainerCell`, or a texture override in `droppedLook`: implementer's
  choice). The save keeps it (a load shows the same chest).
* Draw it with the cursed mimic's texture. If the chest model's UVs do not fit `monsters/mimic_cursed`, make a cursed
  variant of the chest texture from the same look (colours, marks), not a new model.
* Only the cursed mimic. The plain mimic (Chest of Set) and the mimic jar keep their current drop look.
* The map legend (if it marks opened chests) shows it as any chest.

## Play-test

* `bin/game --play tests/play/cursed-mimic-chest-look.txt`: the cursed mimic 4 cells to the right, a death trap
  between (keep off it). Shoot it awake with the bow, then the mace (key) when it comes over.
* The chest it leaves is dark like the mimic (near-black wood, purple panels), not the normal chest. Take the item: it
  stays dark. (Save and load: a test level's save loads campaign level 1, so `tests/unit/cursed_mimic_test.cpp`
  checks it.)

## Done

* `ContainerCell::cursed` (save chars `c`, `y`), `TreasureLook::cursed` from `droppedLook`, set by `dropChest` for the
  cursed mimic only; a dragged chest keeps it (the cell is copied).
* Drawn with `ItemPrototypes::cursedChest`: the chest model, `items/treasure_chest_cursed.png` = the left half of
  `monsters/mimic_cursed.png` (the chest's UVs; `mimic.py --cursed --export` writes it too). No new model.
* The map legend marks it as any treasure (by tile type). Test: `tests/unit/cursed_mimic_test.cpp`.
