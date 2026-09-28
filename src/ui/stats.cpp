#include "stats.h"
#include "../test/scenario.h"
#include <algorithm>
#include <cmath>
#include "../state/game_state.h"
#include "../core/service_locator.h"
#include "../core/logger.h"

void PlayerStats::AddMight(int ns) { Might += ns; }

double PlayerStats::LevelXP(int lvl) { return lvl <= 1 ? 0.0 : 1000 * pow(lvl - 1, 1.4); }

void PlayerStats::AddXP(int xp) {
	XP += xp;
	while (AdvanceLevel())
		;
}

void PlayerStats::SetSprintRequested(bool requested) { sprint_requested = requested; }

bool PlayerStats::IsSprinting() const { return sprinting; }

float PlayerStats::SprintMoveMultiplier() const {
	if (sprinting)
		return 3.0f;

	return 1.0f;
}

void PlayerStats::UpdateStamina() {
	if (!GAME_STATE.player->Alive()) {
		sprinting = false;
		sprint_requested = false;
		return;
	}

	if (!sprint_requested || GAME_STATE.player->Stamina() <= 0) {
		sprinting = false;
		RegenerateStamina();
		return;
	}

	sprinting = true;

	if (!stamina_sprint_drain_timer->TimePassed())
		return;

	stamina_sprint_drain_carry += 0.05f * static_cast<float>(MaxStamina());

	int staminaDrain = static_cast<int>(stamina_sprint_drain_carry);
	if (staminaDrain <= 0)
		return;

	stamina_sprint_drain_carry -= static_cast<float>(staminaDrain);
	if (!GAME_STATE.player->ConsumeStamina(staminaDrain))
		GAME_STATE.player->SetStamina(0);

	if (GAME_STATE.player->Stamina() <= 0) {
		sprinting = false;
	}
}

void PlayerStats::RegenerateStamina() {
	if (!stamina_regen_timer->TimePassed())
		return;

	stamina_regen_carry += 0.02f * static_cast<float>(MaxStamina());

	int staminaGain = static_cast<int>(stamina_regen_carry);
	if (staminaGain <= 0)
		return;

	stamina_regen_carry -= static_cast<float>(staminaGain);
	GAME_STATE.player->AddStamina(staminaGain);
}

int PlayerStats::MaxStamina() const { return 100 + (level - 1) * 10; }

void PlayerStats::Heal(int hpPart) {
	if (HP == MaxHP)
		return;

	float heal = static_cast<float>(hpPart * MaxHP) / static_cast<float>(100.0);
	if (static_cast<float>(HP) + heal > static_cast<float>(MaxHP))
		HP = MaxHP;
	else
		HP = static_cast<int>(heal + static_cast<float>(HP));

	GAME_STATE.player->health = HP;
}

PlayerStats::PlayerStats() {
	MaxHP = 50;
	HP = MaxHP;
	Might = 0;
	Armor = 0;
	level = 1;
	stamina_regen_carry = 0.0f;
	stamina_sprint_drain_carry = 0.0f;
	sprint_requested = false;
	sprinting = false;
	GAME_STATE.player->maxHealth = MaxHP;
	GAME_STATE.player->health = MaxHP;
	GAME_STATE.player->SetMaxStamina(MaxStamina());
	GAME_STATE.player->SetStamina(GAME_STATE.player->MaxStamina());
	stamina_regen_timer = std::make_unique<Timer>(1000);
	stamina_sprint_drain_timer = std::make_unique<Timer>(1000);
}

PlayerStats::~PlayerStats() {
	void* selfPtr = this;
	LOG_DEBUGF("ui", "Deleting stats %p", selfPtr);
}

void PlayerStats::AddArmor(int na) { Armor += na; }

void PlayerStats::TakeHit(int dmg, bool ignoreArmor) {
	if (Scenario::godMode())
		return;

	int damage = std::max(1, ignoreArmor ? dmg : dmg - Armor);

	GAME_STATE.player->takeHit(damage);
	HP = GAME_STATE.player->health;
}

bool PlayerStats::AdvanceLevel() {
	if (XP >= LevelXP(level + 1))
		level++;
	else
		return false;

	if (level % 5 == 0)
		Armor++;

	if (level % 8 == 0)
		Might++;

	MaxHP += 20;
	HP = MaxHP;

	GAME_STATE.player->maxHealth = MaxHP;
	GAME_STATE.player->health = MaxHP;
	GAME_STATE.player->SetMaxStamina(MaxStamina());

	sprintf(GAME_STATE.status, "Now you are level %d\n", level);
	GAME_STATE.status_timer->Reset();

	return true;
}

int PlayerStats::Damage() const {
	return Might + GAME_STATE.ui.inventory->EquippedDamage(); // + weapon dmg, with its level bonus
}

void PlayerStats::AddMaxHP(int hpPart) {
	float more = static_cast<float>(1) + static_cast<float>(hpPart) / static_cast<float>(100.0);
	MaxHP = static_cast<int>(static_cast<float>(MaxHP) * more);
	HP = MaxHP;
	GAME_STATE.player->maxHealth = MaxHP;
	GAME_STATE.player->health = MaxHP;
}

void PlayerStats::Dump(std::ofstream& f) const {
	f << level << " " << XP << " " << Armor << " " << MaxHP << " " << HP << " " << Might << " "
	  << GAME_STATE.player->Stamina() << "\n";
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
	stamina_regen_timer->Reset();
	stamina_sprint_drain_timer->Reset();
	GAME_STATE.player->maxHealth = MaxHP;
	GAME_STATE.player->health = HP;
	GAME_STATE.player->SetMaxStamina(MaxStamina());
	GAME_STATE.player->SetStamina(loadedStamina);
}
