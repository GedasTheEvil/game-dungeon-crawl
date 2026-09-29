#include "player_stats.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include "../state/game_state.h"

namespace {
float ratioOf(int value, int max) {
	return max <= 0 ? 0.f : std::clamp(static_cast<float>(value) / static_cast<float>(max), 0.f, 1.f);
}
} // namespace

void PlayerStats::AddMight(int ns) { Might += ns; }

double PlayerStats::LevelXP(int lvl) { return lvl <= 1 ? 0.0 : 1000 * pow(lvl - 1, 1.4); }

void PlayerStats::AddXP(int xp) {
	XP += xp;
	while (AdvanceLevel())
		;
}

void PlayerStats::UpdateStamina() {
	if (!Alive()) {
		sprinting = false;
		sprint_requested = false;
		return;
	}

	if (!sprint_requested || stamina <= 0) {
		sprinting = false;
		RegenerateStamina();
		return;
	}

	sprinting = true;

	if (!stamina_sprint_drain_timer.TimePassed())
		return;

	stamina_sprint_drain_carry += 0.05f * static_cast<float>(MaxStamina());

	int staminaDrain = static_cast<int>(stamina_sprint_drain_carry);
	if (staminaDrain <= 0)
		return;

	stamina_sprint_drain_carry -= static_cast<float>(staminaDrain);
	if (!ConsumeStamina(staminaDrain))
		SetStamina(0);

	if (stamina <= 0)
		sprinting = false;
}

void PlayerStats::RegenerateStamina() {
	if (!stamina_regen_timer.TimePassed())
		return;

	// 5% of max per second at level 1, +0.5% per level, up to 15%
	const float regenRate = std::min(0.05f + 0.005f * static_cast<float>(level - 1), 0.15f);
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

float PlayerStats::StaminaRatio() const { return ratioOf(stamina, MaxStamina()); }

float PlayerStats::HealthRatio() const { return ratioOf(HP, MaxHP); }

void PlayerStats::Heal(int hpPart) {
	if (HP == MaxHP)
		return;

	float heal = static_cast<float>(hpPart * MaxHP) / static_cast<float>(100.0);
	if (static_cast<float>(HP) + heal > static_cast<float>(MaxHP))
		HP = MaxHP;
	else
		HP = static_cast<int>(heal + static_cast<float>(HP));
}

void PlayerStats::AddArmor(int na) { Armor += na; }

int PlayerStats::HitDamage(int dmg, bool ignoreArmor) const { return std::max(1, ignoreArmor ? dmg : dmg - Armor); }

bool PlayerStats::AdvanceLevel() {
	if (XP >= LevelXP(level + 1))
		level++;
	else
		return false;

	if (level % 5 == 0)
		Armor++;

	if (level % 8 == 0)
		Might++;

	MaxHP += HP_PER_LEVEL;
	HP = MaxHP;
	SetStamina(MaxStamina());
	level_up_ms = GameClock::now();

	Game().ShowStatus("Now you are level %d", level);

	return true;
}

int PlayerStats::Damage() const {
	return Might + Game().ui.inventory->EquippedDamage(); // + weapon dmg, with its level bonus
}

void PlayerStats::AddMaxHP(int hpPart) {
	float more = static_cast<float>(1) + static_cast<float>(hpPart) / static_cast<float>(100.0);
	MaxHP = static_cast<int>(static_cast<float>(MaxHP) * more);
	HP = MaxHP;
}

void PlayerStats::Dump(std::ofstream& f) const {
	f << level << " " << XP << " " << Armor << " " << MaxHP << " " << HP << " " << Might << " " << stamina << "\n";
}

void PlayerStats::LoadDump(std::ifstream& f) {
	f >> level >> XP >> Armor >> MaxHP >> HP >> Might;

	int loadedStamina = 100;

	if (!(f >> loadedStamina))
		f.clear();

	stamina_regen_carry = 0.0f;
	stamina_sprint_drain_carry = 0.0f;
	sprint_requested = false;
	sprinting = false;
	level_up_ms.reset();
	stamina_regen_timer.Reset();
	stamina_sprint_drain_timer.Reset();
	SetStamina(loadedStamina);
}
