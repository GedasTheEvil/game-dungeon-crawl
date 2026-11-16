#ifndef GAMEPLAY_SKILLS_H
#define GAMEPLAY_SKILLS_H

// With the skill the player sprints in half water at this share of the LAND sprint speed (not of the wade speed).
constexpr float WADE_SPRINT_FACTOR_HAPI = 0.5f;
// With the Blessing of Hapi potion (plan water-blessing-potion): the wade is the walk speed, the sprint 40% of the land
// sprint, the jump works. With the potion and the skill: faster than on land, the full land sprint at half the stamina
// drain.
constexpr float WADE_SPEED_FACTOR_BLESSED = 1.0f;
constexpr float WADE_SPRINT_FACTOR_BLESSED = 0.4f;
constexpr float WADE_SPEED_FACTOR_BOTH = 1.25f;
constexpr float WADE_SPRINT_FACTOR_BOTH = 1.0f;
constexpr float WADE_SPRINT_DRAIN_BOTH = 0.5f; // share of the land sprint's stamina drain
constexpr int ARROW_WATER_DAMAGE_PCT = 50;
// A monster entering half water (plan monster-water-splash) splashes when its box is this half wide (map units) or
// more, otherwise it only wades.
constexpr float MONSTER_SPLASH_HALF_WIDTH = 0.3f;
// Fire in water (plan fire-in-water): whoever stands in water takes this share of a hit's fire damage.
// Fire arrows (plan fire-damage): interact by a brazier with a bow, the next shots carry fire, this share of the
// arrow's damage. Mostly fire, so the journal learns the fire resistance from them (mainType).
constexpr int FIRE_ARROW_SHOTS = 5;
constexpr int FIRE_ARROW_PERCENT = 60;
// Flaming arrows (plan skill-flaming-arrows): with the skill and the stamina-skill toggle on, a bow shot deals this
// share of its damage on top as fire, for this share of max stamina. A brazier-lit arrow gets neither.
constexpr int FLAMING_ARROW_BONUS_PERCENT = 40;
constexpr int FLAMING_ARROW_STAMINA_PERCENT = 10;
// Shots (plan ranged-weaker-than-melee): a ranged weapon holds WeaponDef::shots. They come back one every
// SHOT_RECOVERY_MS, the first SHOT_RECOVERY_DELAY_MS after that weapon's last shot (each shot restarts the wait); all
// weapons in the bag at once. A refused shot (none left, a monster too close) tells it at most every
// SHOT_REFUSAL_REPEAT_MS while the key repeats.
constexpr int SHOT_RECOVERY_DELAY_MS = 2000;
constexpr int SHOT_RECOVERY_MS = 2500;
constexpr int SHOT_REFUSAL_REPEAT_MS = 400;
// Power hit (plan skill-power-hit): with the skill and the stamina-skill toggle on, a melee attack held down charges.
// It drains POWER_DRAIN_PERCENT of max stamina a second, up to POWER_CAP_PERCENT a hit (then it holds); the swing at
// the release deals powerHitBonusPercent of the points spent on top. A hold under POWER_TAP_MS is a plain attack, free.
constexpr int POWER_DRAIN_PERCENT = 30;
constexpr int POWER_CAP_PERCENT = 25;
constexpr int POWER_TAP_MS = 150;
// The bonus curve, bonus% = POWER_CURVE_SCALE x ln(1 + points / POWER_CURVE_POINTS): it grows with more points, slower.
constexpr double POWER_CURVE_SCALE = 92.0;
constexpr double POWER_CURVE_POINTS = 20.0;
constexpr int POWER_SHAKE_MS = 250; // the view shakes this long after a power hit,
constexpr float POWER_SHAKE = 1.2f; // up to this many world units at the cap
// Stun (plan skill-stun): with the skill, a weapon hit stuns the monster it does not kill at STUN_CHANCE_PERCENT of the
// weapon's blunt share (a club 25%, a khopesh never), for STUN_MS: it stops moving and attacking. A boss for
// BOSS_STUN_PERCENT of it, and not again within BOSS_STUN_COOLDOWN_MS (no stun-lock).
constexpr int STUN_CHANCE_PERCENT = 25;
constexpr int STUN_MS = 1500;
constexpr int BOSS_STUN_PERCENT = 50;
constexpr int BOSS_STUN_COOLDOWN_MS = 5000;

// Bleed (plan skill-bleed): with the skill, a weapon hit makes the monster it does not kill bleed (a weak bleed) at
// BLEED_CHANCE_PERCENT of the weapon's cut share (a sword 17%, a mace never), then the monster's cut rate scales it
// (Monster::TakeBleed). A boss at half the chance. A monster that already bleeds weak may bleed medium too, at
// BLEED_TIER_UP_PERCENT of the weak chance; one that bleeds medium may bleed strong, at that of the medium chance.
constexpr int BLEED_CHANCE_PERCENT = 20;
constexpr int BOSS_BLEED_PERCENT = 50;
constexpr int BLEED_TIER_UP_PERCENT = 50;
constexpr int WADE_SPLASH_MS = 450; // a splashing step while the player wades
constexpr int WATER_JUMP_HINT_MS = 3000; // "too deep to jump" shows at most this often

#endif
