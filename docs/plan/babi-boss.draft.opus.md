# Babi, the boss of the Lake of Fire

Status: draft 2026-10-21, from the user. Needs first: [baboon](baboon.draft.opus.md) (his kin: model base, ranged
throw, kiting AI). Boss of the Lake of Fire zone ([campaign-zones](solved/campaign-zones.md#zones-decided-2026-10-21)).

## Lore

Babi (Baba), the "bull of the baboons", the dominant male of the baboons in Egyptian myth. He lives by the lake of
fire in the Duat and eats the hearts and entrails of the dead who fail the judgment. The Pyramid Texts ward the dead
king against him; the king also claims his power. A real god's name, so Babi stands as a god like Sobek or Seth.

## Idea

The baboon, stronger in every way (AGENTS.md, Boss abilities):

* **Bigger:** twice the baboon's height, a heavy mane, a red face; the baboon's model scaled and reshaped.
* **Ranged:** a volley of two or three burning coals per throw, farther (6-7 tiles), a shorter cooldown. Or a big
  coal that bursts into a short burning patch on the floor where it lands (implementer's choice).
* **Kiting:** a wider band and a faster retreat than the baboon ([monster-kiting](monster-kiting.draft.opus.md)),
  but within his arena: he leaps from ledge to ledge (a walk-jumper, `Locomotion::WalkJump`) instead of running off.
* **Melee:** a heavy bite and a two-handed blow when cornered.
* **Minions:** baboons climb out over the lake's edge (`Summon`), the boss rule's "plus its kin".
* **Arena:** the sealed boss room is the lake: a pool of fire in the middle (fire on the floor, not water: the burning
  oil of the lake), ledges and ladders round it, water basins at the sides as the player's refuge.
* **Resistances:** `bossResist` of the baboon's (no weakness); fire immune is too much, resists fire.
* **XP, HP, damage:** between Seth and the Anubis boss, tuned by [monster-balance](monster-balance.draft.opus.md).
* **Journal note:** "The heart-eater. I keep mine, thank you."

## Tests

* Unit (sim): the volley, the minion summon, his retreat stays in the arena, the boss gates open on his death.
* Scenario with screenshots: the arena, the volley.
* `bin/levelcheck` with no warnings on his boss level.
