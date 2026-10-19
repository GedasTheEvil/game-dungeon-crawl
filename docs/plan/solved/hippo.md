# Hippo

Status: draft 2026-10-10, from the lore review, picked by the user. Implemented 2026-10-19, see
[Implemented](#implemented); play-tested and solved 2026-10-19 (`bin/game --play tests/play/hippo.txt`, or
`bin/game --level 16`).

## Idea

The hippo, the deadliest animal of the Nile. A water monster: lies submerged in half water (eyes and ears only, like
the crocodile's lurk), then charges along its row through the water.

* Locomotion: submerged lurk, swimmer (`Wading::Swimmer`), and a **charge** like Sobek's (`Charge`): a wind-up, a
  rush through the player, stopped by walls. In half water the wading player cannot jump over it.
* Heavy: high HP, blunt bite, resists blunt; slow on land (it may leave the water, slower).
* Levels: flooded levels after the crocodile's; `levelcheck` requires it in or next to water like the crocodile.
* Boss: none has it as kin. Optional (implementer's choice, a separate step): an Ammit or a great hippo boss later;
  if one gets the hippo as kin, its charge is longer and faster (boss rule).
* Journal: "Taweret's people. On the walls they protect mothers. In the water they protect nothing."
* Glyph: pick a free one. Model: procedural, clips: idle (submerged), rise, wind-up, charge, bite, death.

## Tests

* Unit / sim: lurks, charges along the row, stops at a wall, swims faster in half water.
* Scenario with a screenshot: submerged, charging.

## Implemented

* `MonsterHippo` (27), glyph `o`, game name "Hippopotamus": `Locomotion::Submerged`, `Wading::Swimmer`. Speed 4 on
  land (0.24 tiles/s, slower than the crocodile), 3x in half water (0.72 tiles/s, faster than the wading player);
  280 HP, a 28 bite every 1500 ms (blunt 70, pierce 30), 3600 XP, threat 6. Resists blunt (the fat) and fire (wet
  hide). Journal note as in the draft; its charge is the Charge move ("Charges along the hall. A wall stuns it.").
* The charge per type: `ChargeRules` (`MonsterKind::charge`, was `bool charges`): range and speed. Sobek keeps his
  (2.5-6 tiles, 4 tiles/s); the hippo's is nearer, shorter and slower (`HIPPO_CHARGE_MIN` 2, `HIPPO_CHARGE_MAX` 4,
  `HIPPO_CHARGE_SPEED` 3.2). The windup, the double damage of the hit, the stun in a wall and the cooldown are
  Sobek's. A lurking charger wakes at its charge's far end (4 tiles), so it comes out of the water charging. In half
  water the player cannot jump the charge (no jump while wading); on dry floor a jump clears it.
* Model `tools/blender/models/hippo.py` (walk, attack = the threat yawn and a crushing snap, also the windup; die;
  idle: lying low, only ears, eyes and nostrils over the water), sounds `tools/audio/hippo_sounds.py`. No rise clip:
  a submerged monster has none.
* Levels: one each in the long pools of levels 16, 18, 21 and 23, each in place of a crocodile. `levelcheck`: as the
  crocodile, in or next to water.
* Boss: none has the hippo as kin (an Ammit or a great hippo boss stays a later, separate step).
* Tests: `tests/unit/hippo_test.cpp` (lurks out of range, charges through the player into the wall, slow on land,
  fast in water), scenario `tests/scenarios/solved/hippo.txt` (submerged, windup, charging, stunned).
* Not done: the sounds are synthesized, check them in play. The level sources (`tools/level/campaign/lvl16.txt`, 18,
  21, 23) were already out of step with the level files (a Blessing of Hapi chest each), so the hippo went into both
  by hand.
