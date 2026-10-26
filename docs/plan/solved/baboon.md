# Baboon of the Lake of Fire

Status: draft 2026-10-21, from the user; implemented 2026-10-21, see [Done](#done). Built on [monster-kiting](monster-kiting.md) (it keeps its
distance and shoots). Its boss: [babi-boss](babi-boss.md).

## Lore

The Book of the Dead (chapter 126) and the Amduat put a lake of fire at the gate of the Duat, guarded by four baboons
that sit at its corners. Baboons were sacred to Thoth, and they greet the rising sun with raised hands. Babi, the
"bull of the baboons", eats the hearts of the unworthy by that lake. So the Lake of Fire zone
([campaign-zones](campaign-zones.md#zones-decided-2026-10-21)) is the baboons' home.

## Idea

* **Kind:** a quick walker and climber (`climbs`, ladders), mid HP, a ranged fighter first.
* **Ranged (preferred):** hurls a burning coal from the lake (`RangedRules` with fire in its `mix`, so it sets the
  player burning, drawn as a fireball like the Anubis's bolt). Range about 4-5 tiles, a cooldown of 2-3 s. Water puts
  the burning out, and [fire-in-water](../fire-in-water.md) cuts the fire share of a hit to a tenth in water:
  the lake zone's pools are the player's cover.
* **Melee (fallback):** a bite with long fangs (slash, pierce), when cornered or the player is in reach.
* **AI:** a kiter ([monster-kiting](monster-kiting.md)): keeps the player at its range, backs off when they
  come at it, never far enough to lose them.
* **Tell:** a screech before each throw (its ranged clip lifts the coal over its head first), so the player can dodge
  by a jump or by stepping off the row.
* **Resistances:** resists fire (it lives by the lake), weak to pierce (arrows, javelins), normal otherwise.
* **Journal note**, in the archaeologist's voice: "Thoth's baboons, the priests said. Thoth was a scribe. These
  throw things."
* **Levels:** the Lake of Fire zone, three to five a level; a few in the Necropolis after it. levelgen: the zone's
  monster set ([campaign-levelgen](../campaign-levelgen.md)); until then, from difficulty 9.
* **Glyph:** pick a free one. **Model:** procedural ([modeling.md](../../modeling.md)): a hamadryas baboon, mane on the
  shoulders, a long dog-like muzzle, a crouching walk on all fours. Clips: idle (sitting, hands raised), walk, climb,
  bite, ranged (throw), die. Sounds: screech, bark, throw, die (`tools/audio/`).

## Tests

* Unit (sim): its shot sets the player burning; in water the hit deals a tenth of its fire share (if fire-in-water is
  in); the kiting tests of monster-kiting with the baboon.
* Scenario with screenshots: the throw, the fireball, the bite.
* `bin/levelcheck` with no warnings on the levels that place it.

## Done

* Kind `MonsterBaboon` (31, glyph `z`, `KINDS` in `src/world/monster_kinds.cpp`): speed 22 (a bit faster than the
  player's walk), 110 HP, a bite of 22 every 0.9 s (slash 50 : pierce 50), 3200 XP, threat 5.5 (x1.15 for a kiter),
  resists fire, weak to pierce, a climber. Scale 24: under half the player's height on all fours.
* The coal (`RangedRules`): 24 damage, blunt 30 : fire 70 (sets the player burning, drawn as the Anubis's fireball),
  every 2 s (2.5 s before the play-test: the user wanted it about 20% quicker). Its throw is its own clip (`RangedRules::ownClip`, `THROWER_CLIPS`: `_att_ranged`); the screech and the
  coal over its head are the tell, the coal leaves at 0.57 s.
* **Change from the draft: the range is 1.3 tiles, the band 0.55 to 1.1** (`KiteRules`, back-off speed 28), not
  4-5 tiles. The camera shows only about 1.7 tiles either side of the player (45° at 16:9); at 2.8 it threw from off
  the screen. The other ranged attackers shoot from 1.2 to 1.8.
* Kiting changes (in [monster-kiting](monster-kiting.md#done)): it breaks off its throw clip after the release to back
  off; cornered nearer than its band it closes in and bites; a journal move "Keeps its distance".
* Journal: the note, and its own line for the ranged move ("Screeches, lifts a burning coal over its head ...").
* Levels: the Lake of Fire zone has no levels yet (the rebuild, [campaign-levelgen](../campaign-levelgen.md)),
  so they sit in the necropolis: three in 31, two each in 32 and 33, on long open rows. Levelgen from difficulty 9.
  Not in the zone table's monster sets (the necropolis row is full; the Lake's comes with the rebuild).
* Model `tools/blender/models/baboon.py` (walk, bite, throw, die, idle sitting with the hands raised, climb), sounds
  `tools/audio/baboon_sounds.py` (bark, snarl and snap, screech and whoosh, scream).
* Tests: `tests/unit/baboon_test.cpp` (the kind's rules; the coal burns; in half water the hit lands and the water puts
  the fire out; walked at, it backs off and keeps throwing; a wall behind it, it bites; cornered nearer than its band,
  it closes in and bites; a sprint catches it), on `tests/levels/baboon{,_wall,_water}`; the scenario
  `tests/scenarios/solved/baboon.txt` (the throw, the fireball, sitting in its band, the bite; screenshots).

## Open

* [fire-in-water](../fire-in-water.md) is still a draft: in water the coal's fire share is not cut yet (only
  the burning is put out). Its test goes with that plan.
* The idle clip starts seated: standing up into the walk pops.
