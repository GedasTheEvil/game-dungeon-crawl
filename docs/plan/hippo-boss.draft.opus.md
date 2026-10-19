# Hippo boss: Seth, the red hippopotamus

Status: draft 2026-10-19, from the user ("a hippo boss, Egyptian, stronger than just a hippopotamus"). Ready:
[hippo](solved/hippo.md) is in. Boss slot: not now (user, 2026-10-19), see Placement.

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
* Numbers (starting point, balance in [monster-balance](monster-balance.draft.opus.md)): HP above Sobek's 1600
  (~2000), damage ~80 every 1500 ms, XP ~30000, `Courage::Reckless`, `trapDamagePct` 10, weight 6.
* Fire: the hippo resists it (wet hide); with [fire-in-water](fire-in-water.draft.sonet.md) in, fire barely
  touches him in the water anyway.
* Journal note, in the hippo's tone: "Seth in the shape of the red beast. Horus took him with a harpoon. I have a
  spear; it will have to do."
* Glyph: pick a free one. Sounds: the hippo's, pitched down, plus a roar for the storm.

## Alternative (if the user prefers)

Ammit, the Devourer: crocodile head, lion front, hippo hind. She eats the hearts that fail the weighing before
Osiris. A good fit for the Anubis side of the campaign, but a new model (three animals), so more work; her kin
could be the hippo or the crocodile. Default stays Seth: reuses the hippo model, and the myth is a water fight.

## Placement

No campaign level for now (user, 2026-10-19): the slot is picked later, in the level rebalance with more levels.
It only has to be play-testable: a `tests/play/hippo-boss.txt` script (a flooded room, hippos, Seth), per
[plans.md](../plans.md#implemented). `levelcheck` must accept him (in or next to water, as the hippo).

## Open (warn when reporting)

* Which "stronger" ability (storm roar, wave, or both).

## Tests

* Unit / sim: lurks, charges farther and faster than the hippo, a wall stuns him, the storm roar (or wave) hits and
  pushes; hippos dig out beside him; pierce takes normal damage, blunt resisted.
* Scenario with a screenshot: Seth submerged, charging, stunned, the roar.
