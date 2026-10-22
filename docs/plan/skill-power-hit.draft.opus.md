# Skill: power hit

Status: draft 2026-10-10, refined 2026-10-22 (charged hit, decided by the user). Ready: [skills](solved/skills.md) is
in. Opus: held input, a charge animation and a scaled bonus.

The skill is on the Heka screen already, sealed: implementing it clears its `sealed` in `SKILLS`
(`src/world/skills.cpp`) and checks its card text there.

## Idea

With the skill, a melee attack can be charged: the longer it is held, the more stamina it takes and the harder it hits.
Decided (user, 2026-10-22): the bonus scales with the stamina spent. Not a fixed cost per swing; a fixed cost stays with
[flaming arrows](solved/skill-flaming-arrows.md) only.

* **Melee only.** A bow draw never charges; bows have flaming arrows.
* **Toggle:** the stamina-skills toggle (`GameplayAction::StaminaSkills`, `Player::staminaSkills`), shared with flaming
  arrows. Decided (user, 2026-10-11): never always-on, stamina also pays for sprint and jump. Toggle off: holding the
  attack works as today.
* **Hold to charge** (toggle on): attack becomes a held action, like walk and sprint (`input.cpp` has the key-up and
  mouse-up paths). A tap is today's attack, no cost. Held, the stamina drains and the bonus builds; the release swings.
* **Numbers** (`gameplay_config.h`, in shares of max stamina, so the curve is the same at every level):
  * Drain: 30% of max stamina a second while charging. Cap (user): **25% of max stamina per hit**, full in ~0.85 s;
    held longer, the charge holds at the cap, no further drain.
  * Bonus: 3 × the share spent, linear: 10% → +30%, the cap 25% → +75% of the weapon's damage.
  * A hold under 150 ms is a tap: no cost, no bonus.
  * Stamina runs out mid-charge: the charge stops there, the release swings with what was built.
* **While charging:** no stamina regen, no sprint; walking is allowed.
* **Feedback:** the weapon draws back further as the charge builds; a charge sound rising in pitch; the HUD stamina bar
  shows the drain. On hit: a heavier swing sound, a bigger hit spark, a slight camera shake, all growing with the
  charge.
* **Too little stamina** to start (under the tap threshold's worth): a normal attack, no cost, the HUD toggle sign
  blinks once (`staminaSkillRefusedAgeMs`).

## Open (warn when reporting)

* Numbers need the sim (`monster-balance`): drain rate, cap, multiplier.

## Tests

* Unit / sim: the bonus is 3 × the stamina share spent, capped at 25%; a tap costs nothing and deals normal damage;
  toggle off = today's attack (holding does not charge); a bow draw never charges; empty stamina mid-charge swings with
  the built charge; no regen while charging.
