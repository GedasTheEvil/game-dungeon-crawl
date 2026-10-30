#include "player_stats.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>
#include "../core/gameplay_config.h"
#include "../world/progression.h"

namespace {
float ratioOf(int value, int max) {
	return max <= 0 ? 0.f : std::clamp(static_cast<float>(value) / static_cast<float>(max), 0.f, 1.f);
}
} // namespace

void PlayerStats::AddMight(int ns) { Might += ns; }

double PlayerStats::LevelXP(int lvl) { return levelXP(lvl); }

float PlayerStats::LevelProgress() const { return levelProgress(level, XP); }

int PlayerStats::RiddleXP() const { return riddleXP(level); }

int PlayerStats::InscriptionXP() const { return riddleXP(level) / INSCRIPTION_XP_PART; }

void PlayerStats::AddXP(int xp, WorldEvents& events) {
	XP += xp;
	while (AdvanceLevel(events))
		;
}

void PlayerStats::UpdateStamina(WorldEvents& events) {
	const bool moved = std::exchange(walked, false);
	const bool cheap = std::exchange(walked_cheap, false);
	if (!Alive()) {
		sprinting = false;
		sprint_requested = false;
		return;
	}

	if (charging) { // a power hit's charge (plan skill-power-hit): its own drain, no sprint, no regen
		sprinting = false;
		return;
	}
	// Standing still (or against a wall) with shift held is plain standing: no drain, no refusal.
	if (!sprint_requested || !moved || stamina <= 0) {
		if (sprint_requested && moved)
			RefuseStamina(events);
		sprinting = false;
		RegenerateStamina();
		return;
	}

	sprinting = true;

	if (!stamina_sprint_drain_timer.TimePassed())
		return;

	stamina_sprint_drain_carry += 0.05f * static_cast<float>(MaxStamina()) * (cheap ? WADE_SPRINT_DRAIN_BOTH : 1.f);

	int staminaDrain = static_cast<int>(stamina_sprint_drain_carry);
	if (staminaDrain <= 0)
		return;

	stamina_sprint_drain_carry -= static_cast<float>(staminaDrain);
	if (!ConsumeStamina(staminaDrain))
		SetStamina(0);

	if (stamina <= 0)
		sprinting = false;
}

float PlayerStats::SprintMoveMultiplier() const {
	if (!sprint_requested || stamina <= 0 || charging)
		return 1.f;
	return curses.Has(Curse::Slowness) ? SLOWNESS_SPRINT_MULTIPLIER : SPRINT_MULTIPLIER;
}

float PlayerStats::WalkFactor(bool airborne) const {
	const float rings = airborne ? 1.f : static_cast<float>(100 + this->rings.speedPercent) / 100.f;
	return (curses.Has(Curse::Slowness) ? SLOWNESS_WALK_FACTOR : 1.f) * rings;
}

void PlayerStats::RegenerateStamina() {
	if (!stamina_regen_timer.TimePassed() || curses.Has(Curse::Fatigue))
		return;

	// 5% of max per second at level 1, +0.5% per level, up to 15%
	const float regenRate = std::min(0.05f + 0.005f * static_cast<float>(level - 1), 0.15f) *
							static_cast<float>(100 + rings.staminaRegenPercent) / 100.f; // the sapphire rings'
	stamina_regen_carry += regenRate * static_cast<float>(MaxStamina());

	int staminaGain = static_cast<int>(stamina_regen_carry);
	if (staminaGain <= 0)
		return;

	stamina_regen_carry -= static_cast<float>(staminaGain);
	AddStamina(staminaGain);
}

void PlayerStats::SetStamina(int value) { stamina = std::clamp(value, 0, MaxStamina()); }

bool PlayerStats::ConsumeStamina(int value) {
	if (value <= 0)
		return true;
	if (stamina < value)
		return false;
	stamina -= value;
	return true;
}

void PlayerStats::AddStamina(int value) {
	if (value > 0)
		SetStamina(stamina + value);
}

void PlayerStats::RefuseStamina(WorldEvents& events) {
	stamina_refused_ms = GameClock::now();
	events.Note(FieldNote::Stamina);
}

float PlayerStats::StaminaRatio() const { return ratioOf(stamina, MaxStamina()); }

float PlayerStats::HealthRatio() const { return ratioOf(HP, CurrentMaxHP()); }

void PlayerStats::Heal(int hpPart) {
	const int max = CurrentMaxHP();
	if (HP == max)
		return;

	float heal = static_cast<float>(hpPart * max) / static_cast<float>(100.0);
	if (static_cast<float>(HP) + heal > static_cast<float>(max))
		HP = max;
	else
		HP = static_cast<int>(heal + static_cast<float>(HP));
}

void PlayerStats::AddArmor(int na) { Armor += na; }

int PlayerStats::HitDamage(int dmg, const DamageMix& mix, bool ignoreArmor, bool inWater) const {
	return playerHitDamage(dmg, mix, inWater ? inWaterResist(resist) : resist, CurrentArmor(), ignoreArmor);
}

bool PlayerStats::AdvanceLevel(WorldEvents& events) {
	if (XP >= LevelXP(level + 1))
		level++;
	else
		return false;

	if (level % 5 == 0)
		Armor++;

	if (level % 8 == 0)
		Might++;

	MaxHP += HP_PER_LEVEL;
	HP = CurrentMaxHP();
	poison.Cure(); // a new level heals fully, poison too
	SetStamina(MaxStamina());
	level_up_ms = GameClock::now();

	events.Status("Now you are level %d", level);
	events.Note(FieldNote::Levels);

	return true;
}

void PlayerStats::AddMaxHP(int hpPart) {
	float more = static_cast<float>(1) + static_cast<float>(hpPart) / static_cast<float>(100.0);
	MaxHP = static_cast<int>(static_cast<float>(MaxHP) * more);
	HP = CurrentMaxHP();
}

void PlayerStats::AddMaxStamina(int percent) {
	const int oldMax = MaxStamina();
	max_stamina_percent += percent;
	AddStamina(MaxStamina() - oldMax);
}

std::string PlayerStats::Grant(const Boon& boon) {
	switch (boon.kind) {
	case BoonKind::Might:
		AddMight(boon.amount);
		break;
	case BoonKind::Armor:
		AddArmor(boon.amount);
		break;
	case BoonKind::Life:
		AddMaxHP(boon.amount);
		break;
	case BoonKind::Stamina:
		AddMaxStamina(boon.amount);
		break;
	case BoonKind::None:
		break;
	}
	return boonText(boon);
}

int PlayerStats::TypeMightPercent(const DamageMix& mix) const {
	int sum = 0;
	for (size_t t = 0; t < DAMAGE_TYPE_COUNT; t++)
		sum += mix[t] * amulet.typeMightPercent[t];
	return sum / 100;
}

int PlayerStats::Shots(ItemKind weapon) const {
	const int shots = weaponDef(weapon).shots;
	if (shots == 0 || amulet.shotsPercent == 0)
		return shots;
	return shots + std::max(1, (shots * amulet.shotsPercent + 99) / 100);
}

ShotPace PlayerStats::ShotRecoveryPace() const {
	const int speed = 100 + amulet.shotRecoveryPercent;
	return {SHOT_RECOVERY_DELAY_MS * 100 / speed, SHOT_RECOVERY_MS * 100 / speed};
}

void PlayerStats::Wear(const AmuletBonus& bonus, bool keepShare) {
	const int oldMax = CurrentMaxHP();
	amulet = bonus;
	resist = bonus.resist;
	regen_carry_ms = 0;
	if (keepShare)
		keepHpShare(oldMax);
}

void PlayerStats::keepHpShare(int oldMax) {
	if (HP > 0 && oldMax > 0)
		HP = std::clamp(
			static_cast<int>(std::lround(static_cast<double>(HP) * CurrentMaxHP() / oldMax)),
			1,
			CurrentMaxHP()
		);
}

bool PlayerStats::LayCurse(Curse c, CurseGrade grade, WorldEvents& events) {
	const int oldMax = CurrentMaxHP();
	// Blessing of Isis (plan skill-blessing-of-isis): curses last half as long.
	const int duration = heka.Has(SkillId::BlessingOfIsis) ? CURSE_MS_ISIS : CURSE_MS;
	const bool fresh = curses.Lay(c, grade, duration);
	run.cursed++;
	keepHpShare(oldMax);
	if (c == Curse::Fatigue)
		SetStamina(std::min(stamina, MaxStamina() * FATIGUE_STAMINA_PERCENT / 100));
	const CurseDef& def = curseDef(c);
	events.Status("%s", fresh ? def.startLine : def.againLine);
	events.Note(curseNote(c));
	return true;
}

void PlayerStats::AdvanceCurses(int ms, WorldEvents& events) {
	const int oldMax = CurrentMaxHP();
	const unsigned ended = curses.Advance(ms);
	if (ended == 0)
		return;
	keepHpShare(oldMax);
	for (int c = 0; c < CURSE_COUNT; c++)
		if ((ended & (1U << c)) != 0)
			events.Status("%s", CURSES[static_cast<size_t>(c)].endLine);
}

void PlayerStats::EndCurses() {
	const int oldMax = CurrentMaxHP();
	curses.EndAll();
	keepHpShare(oldMax);
}

int PlayerStats::TrapDamage(int dmg) {
	const int hundredths = dmg * (100 - amulet.trapCutPercent) + trap_carry;
	trap_carry = hundredths % 100;
	return hundredths / 100;
}

void PlayerStats::Regenerate(bool safe, int tickMs) {
	const bool healing = !poison.Any() && Alive() && HP < CurrentMaxHP();
	if (rings.regenHpPerHour <= 0 || !healing)
		ring_regen_carry = 0;
	else { // the rubies, also in a fight
		ring_regen_carry += tickMs * rings.regenHpPerHour;
		HP = std::min(CurrentMaxHP(), HP + ring_regen_carry / RingEffect::MS_PER_HOUR);
		ring_regen_carry %= RingEffect::MS_PER_HOUR;
	}
	if (amulet.regenHpPerSecond <= 0 || !safe || !healing) {
		regen_carry_ms = 0;
		return;
	}
	regen_carry_ms += tickMs * amulet.regenHpPerSecond;
	HP = std::min(CurrentMaxHP(), HP + regen_carry_ms / 1000);
	regen_carry_ms %= 1000;
}

void PlayerStats::WearRings(const RingBonus& bonus) {
	rings = bonus;
	ring_regen_carry = 0;
}

int PlayerStats::PoisonResistPercent() const {
	return std::min(100, amulet.poisonResistPercent + PotionResistPercent());
}

void PlayerStats::AdvanceResistance(int ms) { resist_left_ms = std::max(0, resist_left_ms - ms); }

void PlayerStats::AdvanceWaterBlessing(int ms, WorldEvents& events) {
	if (water_left_ms <= 0)
		return;
	water_left_ms = std::max(0, water_left_ms - ms);
	if (water_left_ms == 0)
		events.Status("The water is deep again");
}

std::string PlayerStats::Drink(const PotionGain& gain) {
	const int hpBefore = HP;
	const int staminaBefore = stamina;
	char buf[64];
	if (gain.healPercent > 0) {
		Heal(gain.healPercent);
		snprintf(buf, sizeof(buf), "Healed %d health", HP - hpBefore);
	} else if (gain.might > 0) {
		AddMight(gain.might);
		snprintf(buf, sizeof(buf), "Might rises to %d", CurrentMight());
	} else if (gain.armor > 0) {
		AddArmor(gain.armor);
		snprintf(buf, sizeof(buf), "Armor rises to %d", CurrentArmor());
	} else if (gain.resistPercent > 0) {
		const bool cured = gain.cure && poison.Any();
		if (gain.cure)
			poison.Cure();
		resist_percent = gain.resistPercent;
		resist_left_ms = PotionEffect::RESIST_MS;
		snprintf(
			buf,
			sizeof(buf),
			"%sResists poison %d%% for %d min",
			cured ? "Cured. " : "",
			PoisonResistPercent(),
			PotionEffect::RESIST_MS / 60000
		);
	} else if (gain.waterBlessing) {
		water_left_ms = PotionEffect::WATER_BLESSING_MS * (heka.Has(SkillId::FavourOfHapi) ? 2 : 1);
		snprintf(buf, sizeof(buf), "The water parts for %d min", water_left_ms / 60000);
	} else if (gain.cure) {
		poison.Cure();
		snprintf(buf, sizeof(buf), "The poison is gone");
	} else if (gain.maxHpPercent > 0) {
		AddMaxHP(gain.maxHpPercent);
		snprintf(buf, sizeof(buf), "Max health rises to %d", CurrentMaxHP());
	} else {
		AddStamina(MaxStamina() * gain.staminaPercent / 100);
		snprintf(buf, sizeof(buf), "Restored %d stamina", stamina - staminaBefore);
	}
	return buf;
}

void PlayerStats::Dump(std::ofstream& f) const {
	f << level << " " << XP << " " << Armor << " " << MaxHP << " " << HP << " " << Might << " " << stamina << "\n";
	poison.Save(f);
	f << "RESIST " << resist_percent << " " << resist_left_ms << "\n";
	f << "WATER " << water_left_ms << "\n";
	f << "BOONS " << max_stamina_percent << "\n";
	heka.Dump(f);
	curses.Dump(f);
	run.Dump(f);
}

void PlayerStats::LoadDump(std::ifstream& f) {
	f >> level >> XP >> Armor >> MaxHP >> HP >> Might;

	int loadedStamina = 100;

	if (!(f >> loadedStamina))
		f.clear();

	stamina_regen_carry = 0.0f;
	stamina_sprint_drain_carry = 0.0f;
	sprint_requested = false;
	walked = false;
	sprinting = false;
	level_up_ms.reset();
	stamina_refused_ms.reset();
	stamina_regen_timer.Reset();
	stamina_sprint_drain_timer.Reset();
	poison.Load(f);
	resist_percent = 0;
	resist_left_ms = 0;
	const auto start = f.tellg();
	std::string tag;
	if (!(f >> tag) || tag != "RESIST") { // a save from before the resistance potions
		f.clear();
		f.seekg(start);
	} else
		f >> resist_percent >> resist_left_ms;
	water_left_ms = 0;
	const auto water = f.tellg();
	if (!(f >> tag) || tag != "WATER") { // a save from before the Blessing of Hapi
		f.clear();
		f.seekg(water);
	} else
		f >> water_left_ms;
	max_stamina_percent = 0;
	const auto boons = f.tellg();
	if (!(f >> tag) || tag != "BOONS") { // a save from before the inscription boons
		f.clear();
		f.seekg(boons);
	} else
		f >> max_stamina_percent;
	SetStamina(loadedStamina); // clamped to the max, which needs the boons
	heka.LoadDump(f);
	curses.LoadDump(f);
	run.LoadDump(f);
}
