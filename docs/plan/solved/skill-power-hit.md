# Skill: power hit

Status: implemented 2026-10-27 ([Implemented](#implemented)), play-tested and solved 2026-10-27:
`bin/game --play tests/play/skill-power-hit.txt`. Was: draft 2026-10-10, refined 2026-10-22 (charged hit, decided by
the user); needed [skills](skills.md).

The skill is on the Heka screen already, sealed: implementing it clears its `sealed` in `SKILLS`
(`src/world/skills.cpp`) and checks its card text there.

## Idea

With the skill, a melee attack can be charged: the longer it is held, the more stamina it takes and the harder it hits.
Decided (user, 2026-10-22): the bonus scales with the stamina spent. Not a fixed cost per swing; a fixed cost stays with
[flaming arrows](skill-flaming-arrows.md) only.

* **Melee only.** A bow draw never charges; bows have flaming arrows.
* **Toggle:** the stamina-skills toggle (`GameplayAction::StaminaSkills`, `Player::staminaSkills`), shared with flaming
  arrows. Decided (user, 2026-10-11): never always-on, stamina also pays for sprint and jump. Toggle off: holding the
  attack works as today.
* **Hold to charge** (toggle on): attack becomes a held action, like walk and sprint (`input.cpp` has the key-up and
  mouse-up paths). A tap is today's attack, no cost. Held, the stamina drains and the bonus builds; the release swings.
* **Numbers** (`gameplay_config.h`). Decided (user, 2026-10-22): the bonus follows the **absolute** stamina points
  spent, not their share, so a higher level with more stamina hits harder for the same 25% charge.
  * Drain: 30% of max stamina a second while charging. Cap (user): **25% of max stamina per hit**, full in ~0.85 s;
    held longer, the charge holds at the cap, no further drain.
  * Bonus, logarithmic (user, 2026-10-22: it grows with more points, but slower): `bonus% = 92 × ln(1 + points / 20)`
    of the weapon's damage. Points spent: 10 → +37%, 25 → +75% (the level 1 cap, 100 max), 50 → +115%,
    100 → +165%, 148 → +196% (the level 50 cap, 590 max).
  * A hold under 150 ms is a tap: no cost, no bonus.
  * Stamina runs out mid-charge: the charge stops there, the release swings with what was built.
* **While charging:** no stamina regen, no sprint; walking is allowed.
* **Feedback:** the weapon draws back further as the charge builds; a charge sound rising in pitch; the HUD stamina bar
  shows the drain. On hit: a heavier swing sound, a bigger hit spark, a slight camera shake, all growing with the
  charge.
* **Too little stamina** to start (under the tap threshold's worth): a normal attack, no cost, the HUD toggle sign
  blinks once (`staminaSkillRefusedAgeMs`).

## Open (warn when reporting)

* Numbers need the sim (`monster-balance`): drain rate, cap, the curve's 92 and 20. The bonus grows with the stamina
  and the weapon both: check high levels (and inscription boons' max stamina) do not one-shot bosses.

## Tests

* Unit / sim: the bonus follows the log curve of the stamina points spent, the spend capped at 25% of max (a higher
  max stamina spends more and hits harder); a tap costs nothing and deals normal damage;
  toggle off = today's attack (holding does not charge); a bow draw never charges; empty stamina mid-charge swings with
  the built charge; no regen while charging.

## Implemented

* The charge lives in `Player` (`StartCharge`, `UpdateCharge`, `ReleaseCharge`, `PowerCharge`), shared by the game
  and `SimWorld`; `Player::CanCharge` is the gate (skill, toggle, melee). Nothing drains in the first 150 ms; after
  that the drain since the press is taken at once, so a release then pays for its whole hold. No regen and no sprint
  while charging (`PlayerStats::SetCharging`).
* Input: the key down starts the charge, the key up asks the next tick to swing (`updatePowerCharge`): a key repeat's
  release and press in between cancel out. The mouse button now charges on down, swings on up; without a charge a click
  still attacks on up, as before. The charged swing starts at the windup (the charge held it there). No weapon switch
  while charging.
* Feedback: the weapon draws back up to 35° further (a thrust: pulls back more), a rising hum
  (`media/sounds/items/power_charge.wav`, `tools/audio/power_hit_sounds.py`) stopped at the release, a deep whoosh, on
  the hit a thump (`WorldSound::PowerHit`), a star spark and a 250 ms view shake (with motion effects on), both grown
  with the charge. The thump's volume does not grow (SDL_mixer plays a chunk at one volume).
* Scenario `attack up` lets the key go; `tests/scenarios/solved/skill-power-hit.txt` shows the pose and the spark.
* Play-test: `bin/game --play tests/play/skill-power-hit.txt`.
