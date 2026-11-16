# Cursed mimic: the chest it leaves keeps its cursed look

Status: draft 2026-11-16, size M, needs nothing first.

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

* `tests/play/cursed-mimic-chest-look.txt`: a cursed mimic next to the start, the player strong enough to kill it.
* Kill it: the chest it leaves has the cursed mimic's look, not the normal chest's. Save, load: still so.
