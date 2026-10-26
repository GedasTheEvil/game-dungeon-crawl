# Hippo boss: Seth, the red hippopotamus

Status: draft 2026-10-19, from the user ("a hippo boss, Egyptian, stronger than just a hippopotamus"). Ready:
[hippo](hippo.md) is in. Boss slot: not now (user, 2026-10-19), see Placement. Implemented 2026-10-20, see
[Implemented](#implemented); play-test: `bin/game --play tests/play/hippo-boss.txt`.

## Myth

In the Contendings of Horus and Seth, Seth turns into a red hippopotamus and fights Horus under the water of the
Nile. On the walls of Edfu, Horus stands in his boat and harpoons the red hippo, and the king spears it in the
rite each year. Seth is the god of storms, the desert and chaos; red was his colour. The hippo was his beast, the
dangerous male of the river (Taweret, the hippo mother, protects; the bull hippo is Seth's).

So the boss is not a big hippo: it is a god in hippo shape, red, raging, with a storm about him.

## Idea

`MonsterSeth`, kin `MonsterHippo` (boss rule: the hippo's abilities, stronger).

* Model: the hippo's (all clips), scale ~1.7x as Sobek on the crocodile. Own texture `hippo_seth`: deep red hide,
  black mottling, a gold-and-carnelian collar, pale eyes. Implementer's choice: the Seth-animal ears (tall, square
  tipped) as a small model variant, if cheap.
* Submerged lurk, swimmer, charge (as the hippo): longer and faster. Default: 2.5 to 7 tiles at 4.5 tiles/s (Sobek:
  2.5-6 at 4, the hippo: 2-4 at 3.2). A wall stuns him (as Sobek).
* Stronger than a hippo, beyond the charge (the "more than a hippo" part). Default, one or both (implementer's
  choice):
  * **Storm roar:** after a stun ends (the wall hit), he roars: a short sandstorm burst around him, a cone or a
    ring of 2 tiles, blunt damage and a push back; sand blurs the screen for ~1 s. Seth's storm.
  * **Wave:** surfacing from a lurk (or each charge's start) throws a wave down his row in half water: the wading
    player is pushed and slowed (stamina drain, as the lotus bite). The flood he swims in.
* Weakness, from the myth: a spear (pierce) is his bane. Default: his pierce rate stays `NORMAL` (not `RESISTS`
  like his blunt), and the journal note hints at it. `bossResist` already lifts the hippo's weaknesses.
* Minions: hippos surfacing from the water (`Summon::DigOut` splash, as Sobek's crocodiles). Fewer than Sobek's:
  default 1 / 2 / every 7 s / 4 per fight (a hippo is heavy).
* Numbers (starting point, balance in [monster-balance](../monster-balance.md)): HP above Sobek's 1600
  (~2000), damage ~80 every 1500 ms, XP ~30000, `Courage::Reckless`, `trapDamagePct` 10, weight 6.
* Fire: the hippo resists it (wet hide); with [fire-in-water](fire-in-water.md) in, fire barely
  touches him in the water anyway.
* Journal note, in the hippo's tone: "Seth in the shape of the red beast. Horus took him with a harpoon. I have a
  spear; it will have to do."
* Glyph: pick a free one. Sounds: the hippo's, pitched down, plus a roar for the storm.

## Alternative (if the user prefers)

Ammit, the Devourer: crocodile head, lion front, hippo hind. She eats the hearts that fail the weighing before
Osiris. A good fit for the Anubis side of the campaign, but a new model (three animals), so more work; her kin
could be the hippo or the crocodile. Default stays Seth: reuses the hippo model, and the myth is a water fight.

## Placement

No campaign level in this plan (user, 2026-10-19). His slot: the boss of the
[desert-chapter](desert-chapter.md) (lvl25 there, his oasis). Here he only has to be play-testable: a `tests/play/hippo-boss.txt` script (a flooded room, hippos, Seth), per
[plans.md](../../plans.md#implemented). `levelcheck` must accept him (in or next to water, as the hippo).

## Open (warn when reporting)

* Which "stronger" ability (storm roar, wave, or both).

## Tests

* Unit / sim: lurks, charges farther and faster than the hippo, a wall stuns him, the storm roar (or wave) hits and
  pushes; hippos dig out beside him; pierce takes normal damage, blunt resisted.
* Scenario with a screenshot: Seth submerged, charging, stunned, the roar.

## Implemented

* `MonsterSeth` (30), glyph `L`, game name "Seth": the hippo model at scale 88 (1.7 x the hippo's 52), texture
  `hippo_seth` (`tools/blender/models/hippo.py --boss-texture`: deep red hide, black mottling, a gold collar with a
  carnelian band, pale eyes). No Seth-animal ears.
* Lurk, swim and charge as the hippo: `SETH_CHARGE_MIN` 2.5, `SETH_CHARGE_MAX` 7 tiles at `SETH_CHARGE_SPEED` 4.5
  tiles/s; a wall stuns him (Sobek's stun). He wakes from `BOSS_WAKE_RANGE` (4 tiles), as Sobek.
* The stronger ability: the **storm roar** only (no wave). `RoarRules` (`MonsterKind::roar`): as the stun ends
  (`Monster::UpdateCharge` returns true) `Dungeon::stormRoar` hits the player on his row within 2 tiles of his box
  (45 blunt, `ROAR_MIX`; a dodge avoids the hit, not the shove), shoves them 1.5 tiles away (`Dungeon::ShovePlayer`:
  a slide at `SHOVE_SPEED` 6 tiles/s, walls stop it) and covers the screen with sand for 1 s (`Dungeon::SandHaze`, a
  sand-coloured overlay in `draw.cpp`, fading). Journal move `CreatureMove::Roar`.
* Pierce `NORMAL`, blunt and fire `RESISTS` (`bossResist(HIPPO_RESIST)`); the journal note hints at the spear.
* Minions: hippos dig out, 1 / 2 / every 7 s / 4 per fight. HP 2000, 80 damage every 1500 ms, 30000 XP, threat 16,
  `Courage::Reckless`, `trapDamagePct` 10, weight 6.
* Sounds: `tools/audio/seth_sounds.py` (the hippo's wake, bite and death pitched down to 0.78, and `seth_roar.wav`: a
  bellow swelling into a gust of sand). A boss's own voice on its kin's model: `MonsterKind::sounds`;
  `CharacterSound::Roar` (`_roar.wav`).
* Tests: `tests/unit/seth_test.cpp` (his rules against the hippo's and Sobek's, the charge into the wall and the
  roar's hit, shove and haze, no roar out of its radius, hippos dig out), scenario `tests/scenarios/solved/seth.txt`
  (submerged, windup, charging, stunned, the roar's sand), level `tests/levels/seth`, play script
  `tests/play/hippo-boss.txt`.
* No campaign level here: his oasis came with [desert-chapter](desert-chapter.md), lvl25.
