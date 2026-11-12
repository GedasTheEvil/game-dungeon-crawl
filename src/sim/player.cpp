#include "player.h"
#include <algorithm>
#include <cmath>
#include "figures.h"
#include "../level/gameplay_config.h"
#include "../level/head_bump.h"
#include "render_config.h"

int powerHitBonusPercent(int points) {
	if (points <= 0)
		return 0;
	return static_cast<int>(
		std::lround(POWER_CURVE_SCALE * std::log1p(static_cast<double>(points) / POWER_CURVE_POINTS))
	);
}

namespace {
int powerCap(const PlayerStats& stats) { return std::max(1, stats.MaxStamina() * POWER_CAP_PERCENT / 100); }
// Stamina points drained after held ms, before the cap.
int powerDrain(const PlayerStats& stats, int heldMs) {
	return static_cast<int>(static_cast<long long>(stats.MaxStamina()) * POWER_DRAIN_PERCENT * heldMs / 100000);
}
} // namespace

bool Player::StartCharge() {
	if (stats.Stamina() < std::max(1, powerDrain(stats, POWER_TAP_MS))) {
		staminaSkillRefusedMs = GameClock::now();
		return false;
	}
	charge = {GameClock::now(), 0, false};
	stats.SetCharging(true);
	return true;
}

// Nothing in the first POWER_TAP_MS: a release then is a tap. After it, the drain since the key went down at once.
void Player::UpdateCharge() {
	if (!Charging() || charge.dry)
		return;
	const int held = GameClock::now() - charge.startMs;
	if (held < POWER_TAP_MS)
		return;
	const int want = std::min(powerCap(stats), powerDrain(stats, held)) - charge.spent;
	const int take = std::min(want, stats.Stamina());
	if (take > 0 && stats.ConsumeStamina(take))
		charge.spent += take;
	charge.dry = take < want;
}

int Player::ReleaseCharge() {
	UpdateCharge();
	swingPower = GameClock::now() - charge.startMs < POWER_TAP_MS ? 0 : charge.spent;
	charge = {};
	stats.SetCharging(false);
	return swingPower;
}

float Player::ChargeShare() const {
	return std::min(1.f, static_cast<float>(charge.spent) / static_cast<float>(powerCap(stats)));
}

float Player::SwingShare() const {
	return std::min(1.f, static_cast<float>(swingPower) / static_cast<float>(powerCap(stats)));
}

void Player::SetModel(const ModelInfo& info) {
	model = info;
	for (AnimPlayback& p : playback)
		p.stepStart = GameClock::now();
	state = model.reference;
}

float Player::HalfWidth() const { return model.HalfWidth() * scale * Figures::Scale() / RenderConfig::TILE_SIZE; }

float Player::StandingHeight() const { return model.referenceTop * scale * Figures::Scale() / RenderConfig::TILE_SIZE; }

float Player::Height() const { return StandingHeight() * (crouched ? CROUCH_HEIGHT_FACTOR : 1.f); }

// The blood runs twice a tick while alive (and is drawn twice), once when dead: as it always has.
void Player::Animate() {
	if (Alive()) {
		if (state == ModelState::Die) // healed back to life
			setModelState(ModelState::Idle);
		blood.Explode();
		blood.Fall();
	} else
		setModelState(ModelState::Die);
	blood.Explode();
	blood.Fall();
	if (state != ModelState::Climb && state != ModelState::CrouchDown) // their frame follows the height / the phase
		model.Advance(state, playback);
	shownFrame = static_cast<int>(playback[static_cast<int>(model.Shown(state))].frame);
}

int Player::TakeHit(int dmg, const DamageMix& mix, WorldEvents& events, bool ignoreArmor, int burnMs) {
	Vitals& v = stats.vitals;
	const Harm harm{.damage = dmg, .mix = mix, .ignoreArmor = ignoreArmor, .burnMs = burnMs};
	if (const double fire = v.FireOf(harm, stats.resist); fire > 0.0 && Alive()) {
		if (!v.burning.Any())
			events.Status("You are on fire!");
		v.Ignite(fire, burnMs, harm.source);
		events.Note(FieldNote::Fire);
	}
	if (god)
		return 0;
	const int s = static_cast<int>(scale);
	int lost = 0;
	if (Alive()) {
		lost = v.Lose(v.Cost(harm, stats.HitGuard()));
		stats.run.damageTaken += lost;
		blood.Splash(s);
		events.Note(FieldNote::Health);
	}

	if (!Alive() && state != ModelState::Die) {
		die(events);
		blood.Splash(s);
		for (int i = 0; i < 6; i++)
			blood.Explode();
	}
	return lost;
}

bool Player::Dodges(WorldEvents& events, Rng& rng) {
	if (!Alive() || !stats.heka.Has(SkillId::Dodge) || !rng.percent(DODGE_PERCENT))
		return false;
	events.Status("Dodged");
	events.Play(WorldSound::Dodge);
	return true;
}

void Player::die(WorldEvents& events) {
	setModelState(ModelState::Die);
	events.PlayCharacter(PLAYER_CHARACTER, CharacterSound::Die);
	stats.vitals.poison.Cure();
	stats.EndResistance();
	stats.EndWaterBlessing();
	stats.EndBlessings();
	stats.EndCurses();
	stats.vitals.burning.Douse();
}

bool Player::Curse(const CurseEntries& entries, WorldEvents& events, Rng& rng) {
	if (!Alive())
		return false;
	for (const CurseEntry& e : entries)
		if (e.percent > 0 && rng.percent(e.percent))
			return stats.LayCurse(e.kind, e.grade, events, rng);
	return false;
}

void Player::Poison(PoisonTier tier, WorldEvents& events, Rng& rng) {
	if (!Alive())
		return;
	if (stats.Blessed(God::Serket)) { // plan shrines-of-the-gods: no poison lands, from any source
		events.Status(StatusTone::Good, "Serket turns the venom away");
		return;
	}
	const bool poisoned = stats.vitals.poison.Any();
	if (!stats.vitals.TakePoison( // the Serket's favour skill: half the time (plan skill-serkets-favour)
			tier,
			stats.PoisonResistPercent(),
			rng,
			HarmSource::Monster,
			stats.heka.Has(SkillId::SerketsFavour)
		)) {
		// The source named, plan skill-god-resist: the potion, else the amulet, else only the skill.
		const char* line = "Serket's favour spares you the venom";
		if (stats.PotionResistPercent() > 0)
			line = "You resist the poison";
		else if (stats.AmuletPoisonResistPercent() > 0)
			line = "Your amulet wards off the poison";
		events.Status("%s", line);
		return;
	}
	if (!poisoned)
		events.Status(StatusTone::Danger, "You are poisoned!");
	stats.run.poisoned++;
	events.Note(FieldNote::Poison);
}

void Player::UpdatePoison(WorldEvents& events) {
	stats.AdvanceResistance(UPDATE_TICK_MS);
	stats.AdvanceWaterBlessing(UPDATE_TICK_MS, events);
	stats.AdvanceBlessings(UPDATE_TICK_MS, events);
	stats.AdvanceCurses(UPDATE_TICK_MS, events);
	const int hp = stats.vitals.PoisonTick(UPDATE_TICK_MS, stats.CurrentMaxHP(), stats.PoisonFasterPercent());
	if (hp <= 0 || !Alive() || god)
		return;
	stats.LoseHP(hp); // armour does not help
	if (!Alive())
		die(events);
}

void Player::UpdateBurning(bool wading, WorldEvents& events) {
	Vitals& v = stats.vitals;
	v.inWater = wading;
	if (v.inWater && v.burning.Any())
		events.Status("The water puts the fire out");
	const int hp = v.BurnTick(UPDATE_TICK_MS);
	if (hp <= 0 || god)
		return;
	stats.LoseHP(hp); // armour does not help
	if (!Alive())
		die(events);
}

void Player::Blind(int ms, WorldEvents& events) {
	if (!Alive())
		return;
	if (blindMs <= 0)
		events.Status("Venom in your eyes!");
	blindMs = std::max(blindMs, ms);
}

void Player::Daze(int ms, WorldEvents& events) {
	if (!Alive())
		return;
	if (!Dazed())
		events.Status("Your ears ring!");
	stats.vitals.KnockOut(ms);
}

int Player::HeadBump(WorldEvents& events, Rng& rng) {
	if (!Alive())
		return 0;
	events.PlayCharacter(PLAYER_CHARACTER, CharacterSound::Attack); // the grunt (the player has no hurt sound)
	events.Status("You bump your head");
	const HeadBumpDaze roll = rollHeadBumpDaze(rng);
	int lost = 0;
	if (!god) { // the daze also lands in god mode, as a screech's does
		lost = stats.vitals.Lose(headBumpHp(stats.CurrentMaxHP()));
		stats.run.damageTaken += lost;
		blood.Splash(static_cast<int>(scale));
		events.Note(FieldNote::Health);
		if (!Alive()) {
			die(events);
			return lost;
		}
	}
	if (roll.daze)
		Daze(roll.ms, events);
	return lost;
}

void Player::UpdateSenses() {
	blindMs = std::max(0, blindMs - UPDATE_TICK_MS * (stats.vitals.inWater ? BLIND_WATER_FACTOR : 1));
}

float Player::Daziness() const {
	return std::min(1.f, static_cast<float>(stats.vitals.OutLeftMs()) / static_cast<float>(DAZE_CLEAR_MS));
}

float Player::Blindness() const {
	return std::min(1.f, static_cast<float>(blindMs) / static_cast<float>(BLIND_CLEAR_MS));
}

void Player::Reanimate() {
	stats.vitals.burning.Douse();
	stats.vitals.ComeTo();
	blindMs = 0;
	fireArrows = 0;
	staminaSkills = false;
	staminaSkillRefusedMs.reset();
	charge = {};
	swingPower = 0;
	stats.SetCharging(false);
	stats.EndCurses();
	stats.EndBlessings(); // before the heal: Amun's raised max goes first
	stats.HealFully();
	stats.vitals.poison.Cure();
	stats.EndResistance();
	stats.EndWaterBlessing();
	crouched = false;
	crouchPhase = 0.f;
	setModelState(model.reference);
	blood.Stop(); // a new or loaded game starts without the last game's splash
}

void Player::showClimb(float phase) {
	const ClipInfo& climb = model.Clip(ModelState::Climb);
	if (!climb.present)
		return;
	state = ModelState::Climb;
	int frames = climb.frames;
	float frame = std::min(phase - std::floor(phase), 1.f) * static_cast<float>(frames);
	playback[static_cast<int>(ModelState::Climb)] = {std::min(frame, static_cast<float>(frames) - 0.001f), 0};
}

void Player::UpdateCrouchPose(bool walking) {
	const float step = static_cast<float>(UPDATE_TICK_MS) / static_cast<float>(CROUCH_TRANSITION_MS);
	crouchPhase = std::clamp(crouchPhase + (crouched ? step : -step), 0.f, 1.f);
	const ClipInfo& down = model.Clip(ModelState::CrouchDown);
	if (crouchPhase > 0.f && crouchPhase < 1.f && down.present) {
		state = ModelState::CrouchDown;
		const float last = static_cast<float>(down.frames - 1);
		playback[static_cast<int>(ModelState::CrouchDown)] = {crouchPhase * last, 0};
	} else if (crouched)
		setModelState(walking ? ModelState::CrouchWalk : ModelState::Crouch);
	else if (crouchPosed())
		setModelState(walking ? ModelState::Move : ModelState::Idle);
}
