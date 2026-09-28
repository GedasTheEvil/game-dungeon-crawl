#ifndef STATS_H
#define STATS_H
#include "../entities/monster.h"
#include "../entities/item.h"
#include "inventory.h"
#include "fstream"
#include <memory>

class PlayerStats {
  private:
	int level;
	double XP;
	bool AdvanceLevel();

	int Armor;
	int MaxHP;
	int HP;
	int Might;

	std::unique_ptr<Timer> stamina_regen_timer;
	std::unique_ptr<Timer> stamina_sprint_drain_timer;
	float stamina_regen_carry;
	float stamina_sprint_drain_carry;
	bool sprint_requested;
	bool sprinting;

  public:
	void SetSprintRequested(bool requested);
	[[nodiscard]] bool IsSprinting() const;
	[[nodiscard]] float SprintMoveMultiplier() const;
	void UpdateStamina();
	void RegenerateStamina();
	[[nodiscard]] int MaxStamina() const;
	[[nodiscard]] int Damage() const;
	[[nodiscard]] int CurrentMight() const { return Might; }
	[[nodiscard]] int CurrentArmor() const { return Armor; }
	[[nodiscard]] int CurrentHP() const { return HP; }
	[[nodiscard]] int CurrentMaxHP() const { return MaxHP; }
	[[nodiscard]] int CurrentLevel() const { return level; }
	[[nodiscard]] double CurrentXP() const { return XP; }
	// XP total at which the player reaches `lvl` (level 2 at 1000).
	[[nodiscard]] static double LevelXP(int lvl);
	void AddMight(int ns = 1);
	void AddXP(int xp);
	void Heal(int hpPart);
	PlayerStats();
	~PlayerStats();
	void AddArmor(int na = 1);
	void TakeHit(int dmg, bool ignoreArmor = false);
	void AddMaxHP(int hpPart);
	void Dump(std::ofstream& f) const;
	void LoadDump(std::ifstream& f);
};

#endif
