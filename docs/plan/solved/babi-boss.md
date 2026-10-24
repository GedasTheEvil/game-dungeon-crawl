# Babi, the boss of the Lake of Fire

Status: draft 2026-10-21, from the user. Needs first: [baboon](baboon.md) (his kin: model base, ranged
throw, kiting AI). Implemented 2026-10-24, see [Implemented](#implemented-2026-10-24); play-test:
`bin/game --play tests/play/babi-boss.txt`; play-tested and solved 2026-10-24. Boss of the Lake of Fire zone ([campaign-zones](campaign-zones.md#zones-decided-2026-10-21)).

## Lore

Babi (Baba), the "bull of the baboons", the dominant male of the baboons in Egyptian myth. He lives by the lake of
fire in the Duat and eats the hearts and entrails of the dead who fail the judgment. The Pyramid Texts ward the dead
king against him; the king also claims his power. A real god's name, so Babi stands as a god like Sobek or Seth.

## Idea

The baboon, stronger in every way (AGENTS.md, Boss abilities):

* **Bigger:** twice the baboon's height, a heavy mane, a red face; the baboon's model scaled and reshaped.
* **Ranged, both throws alternating (user, 2026-10-22):** usually a volley of two or three burning coals, farther
  (6-7 tiles), a shorter cooldown; every few throws a big coal that bursts into a short burning patch on the floor
  where it lands.
* **Kiting:** a wider band and a faster retreat than the baboon ([monster-kiting](monster-kiting.md)),
  but within his arena: he leaps from ledge to ledge (a walk-jumper, `Locomotion::WalkJump`) instead of running off.
* **Melee:** a heavy bite and a two-handed blow when cornered.
* **Minions:** baboons climb out over the lake's edge (`Summon`), the boss rule's "plus its kin".
* **Arena:** the sealed boss room is the lake: a pool of fire in the middle (fire on the floor, not water: the burning
  oil of the lake), ledges and ladders round it, water basins at the sides as the player's refuge.
* **Resistances:** `bossResist` of the baboon's (no weakness); fire immune is too much, resists fire.
* **XP, HP, damage:** between Seth and the Anubis boss, tuned by [monster-balance](../monster-balance.draft.opus.md).
* **Journal note:** "The heart-eater. I keep mine, thank you."

## Tests

* Unit (sim): the volley, the minion summon, his retreat stays in the arena, the boss gates open on his death.
* Scenario with screenshots: the arena, the volley.
* `bin/levelcheck` with no warnings on his boss level.

## Implemented (2026-10-24)

* `MonsterBabi` (34, glyph `]`), kin and minion `MonsterBaboon`, `Summon::DigOut` (2 at once, up to 4, every 5 s, 8 a
  fight). 2200 HP, 110 damage every 1.2 s (`BABI_MIX`: blunt 40, slash 30, pierce 30), 30000 XP, threat 16, speed 26
  (the baboon's 22), `bossResist(BABOON_RESIST)`: resists fire, the weakness to pierce gone. A climber, a walk-jumper
  (`Locomotion::WalkJump`), reckless (traps), as the other bosses.
* Model: `baboon.py --boss` (or `BABOON_BOSS=1`) builds the baboon reshaped into his own files `babi*.md3` and
  `babi.png`: a heavier mane down to the loins with longer and more locks, bigger cheek tufts, a blood-red face, darker
  fur; scale 48, twice the baboon's. His attack clip rears with both hands high, slams them down, then bites: **change
  from the draft**: one attack (one hit), its clip showing the two-handed blow and the bite, not two moves. A leap clip
  (`_jump`; `THROWER_CLIPS` has an optional `_jump`, walk-jumpers with a throw use them). Voice `tools/audio/babi_sounds.py`
  (`babi_{wake,att,att_ranged,jump,die}.wav`).
* Throws (`VolleyRules` in `RangedRules::volley`): every 1.6 s (the baboon's 2 s), a volley of two or three coals 0.25 s
  apart, each aimed at the player's chest as it leaves (`Dungeon::aimShot`); every third throw (`Monster::BigThrow`)
  one big coal (drawn twice the size) at the player's feet, 90 damage: where it lands, on the player or on the floor,
  the floor of that cell burns for 4 s (`Dungeon::Blaze`, `dungeon_blaze.cpp`, drawn in `dungeon_render_blaze.cpp`):
  the player standing in it, not in a jump, takes `BLAZE_DAMAGE` 30 (fire) every 0.5 s and burns on; never in half
  water. **Change from the draft: the range is 1.4 tiles** (the baboon's 1.3), not 6-7: the camera shows about 1.7
  tiles either side of the player, the baboon's reason (its plan).
* Kiting: band 0.6 to 1.25 (the baboon's 0.55 to 1.1), backing off at 40 (the baboon's 28; a sprint, 57, still
  catches him). With a gap behind him (`Dungeon::leapAway`) he leaps it to the next ledge: when the player comes at
  him and he has no room to back off, or when he is nearer than his band with the gap behind (then only if he lands
  in his band, so he does not leap back and forth). Never off his row. A boss and its minions do not corner each
  other in `Dungeon::kiteRoom` (his baboons spawn behind him and kept him from backing off).
* Arena `tests/levels/babi` (no warnings): the teleporter into the sealed room, five ledges with one-cell gaps (he
  leaps them, the player jumps them), the lake of fire a row below, ladders up out of it, water basins at both ends,
  the boss gate and the exit. **Change from the draft: the lake's fire is fire vents** (pairs at alternate phases
  under each gap), not a burning floor: there is no such tile. No campaign slot: the Lake of Fire's levels (42-45)
  come with [campaign-levelgen](../campaign-levelgen.draft.opus.md); placed there by the rebuild.
* Journal: the note; the volley's line for his throw, moves `Blaze` and `LeapAway`, his summon "His baboons climb up
  out of the lake at his call" (they dig out, as the draft's "climb out over the lake's edge" has no summon of its own).
* Tests: `tests/unit/babi_test.cpp` (the kind's rules; volleys and every third throw the big coal; the big coal sets
  the floor at the player's feet burning; the burning floor hurts and sets burning, a jump clears it, never in water;
  walked at, he leaps the gap behind him and stays on his row; in his arena his death opens the boss gate), on
  `tests/levels/babi{,_hall,_ledge}`; the scenario `tests/scenarios/solved/babi.txt`, parked once solved (the arena, his baboons, the volley;
  screenshots); play script `tests/play/babi-boss.txt`.
