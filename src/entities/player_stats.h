#ifndef PLAYER_STATS_H
#define PLAYER_STATS_H

#include "../core/timer.h"
#include <fstream>

// The player's level, XP, might, armour, HP and stamina (sprint drains it, it regenerates). Saved with the game.
class PlayerStats {
  private:
	int level = 1;
	double XP = 0;
	int Armor = 0;
	int MaxHP = 50;
	int HP = 50;
	int Might = 0;
	int stamina = 100;

	Timer stamina_regen_timer{1000};
	Timer stamina_sprint_drain_timer{1000};
	float stamina_regen_carry = 0.f;
	float stamina_sprint_drain_carry = 0.f;
	bool sprint_requested = false;
	bool sprinting = false;

	bool AdvanceLevel();
	void RegenerateStamina();

  public:
	void SetSprintRequested(bool requested) { sprint_requested = requested; }
	[[nodiscard]] bool IsSprinting() const { return sprinting; }
	[[nodiscard]] float SprintMoveMultiplier() const { return sprinting ? 3.f : 1.f; }
	void UpdateStamina();
	[[nodiscard]] int Stamina() const { return stamina; }
	[[nodiscard]] int MaxStamina() const { return 100 + (level - 1) * 10; }
	void SetStamina(int value);
	bool ConsumeStamina(int value); // false (and nothing spent) if there is not enough
	void AddStamina(int value);
	[[nodiscard]] float StaminaRatio() const;

	[[nodiscard]] int Damage() const; // might plus the equipped weapon's damage
	[[nodiscard]] int CurrentMight() const { return Might; }
	[[nodiscard]] int CurrentArmor() const { return Armor; }
	[[nodiscard]] int CurrentHP() const { return HP; }
	[[nodiscard]] int CurrentMaxHP() const { return MaxHP; }
	[[nodiscard]] int CurrentLevel() const { return level; }
	[[nodiscard]] double CurrentXP() const { return XP; }
	[[nodiscard]] bool Alive() const { return HP > 0; }
	[[nodiscard]] float HealthRatio() const;
	// XP total at which the player reaches `lvl` (level 2 at 1000).
	[[nodiscard]] static double LevelXP(int lvl);
	void AddMight(int ns = 1);
	void AddXP(int xp);
	void Heal(int hpPart); // percent of max HP
	void HealFully() { HP = MaxHP; }
	void AddArmor(int na = 1);
	// HP lost to a hit of dmg: armour absorbs some, unless ignoreArmor; at least 1.
	[[nodiscard]] int HitDamage(int dmg, bool ignoreArmor) const;
	void LoseHP(int hp) { HP -= hp; }
	void AddMaxHP(int hpPart); // percent, heals fully
	void Dump(std::ofstream& f) const;
	void LoadDump(std::ifstream& f);
};

#endif
