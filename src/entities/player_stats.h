#ifndef PLAYER_STATS_H
#define PLAYER_STATS_H

#include "../core/timer.h"
#include "../world/damage.h"
#include "../world/item_bag.h"
#include "../world/poison.h"
#include "../world/world_events.h"
#include <fstream>
#include <optional>
#include <string>

constexpr int HP_PER_LEVEL = 12; // max HP gained per level up

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
	AmuletBonus amulet;		// the worn amulet's (Wear); Armor, MaxHP and Might above are without it
	int regen_carry_ms = 0; // regeneration time not yet turned into HP
	int trap_carry = 0;		// hundredths of trap damage not yet taken (TrapDamage)

	Timer stamina_regen_timer{1000};
	Timer stamina_sprint_drain_timer{1000};
	float stamina_regen_carry = 0.f;
	float stamina_sprint_drain_carry = 0.f;
	bool sprint_requested = false;		   // shift held
	bool walked = false;				   // the player took a walk step this tick (NoteWalked)
	bool sprinting = false;				   // shift held and the player moved last tick: stamina drains
	std::optional<int> level_up_ms;		   // game clock time of the last level up, for the sun beam
	std::optional<int> stamina_refused_ms; // last jump or sprint refused for lack of stamina, for the HUD flash

	bool AdvanceLevel(WorldEvents& events);
	void RegenerateStamina();

  public:
	void SetSprintRequested(bool requested) { sprint_requested = requested; }
	[[nodiscard]] bool IsSprinting() const { return sprinting; }
	// Shift held with stamina left: the step is already a sprint step, before UpdateStamina sees it moved.
	[[nodiscard]] float SprintMoveMultiplier() const { return sprint_requested && stamina > 0 ? 3.f : 1.f; }
	void NoteWalked() { walked = true; } // a walk step really moved the player (not into a wall)
	void UpdateStamina(WorldEvents& events);
	[[nodiscard]] int Stamina() const { return stamina; }
	[[nodiscard]] int MaxStamina() const { return 100 + (level - 1) * 10; }
	void SetStamina(int value);
	bool ConsumeStamina(int value); // false (and nothing spent) if there is not enough
	void AddStamina(int value);
	[[nodiscard]] float StaminaRatio() const;
	void RefuseStamina(WorldEvents& events); // a jump or sprint wanted more stamina than there is
	[[nodiscard]] std::optional<int> StaminaRefusedMs() const { return stamina_refused_ms; }

	[[nodiscard]] int Damage(int weaponDamage) const { return CurrentMight() + weaponDamage; }
	// With the worn amulet's bonus.
	[[nodiscard]] int CurrentMight() const { return Might + amulet.might; }
	[[nodiscard]] int CurrentArmor() const { return Armor + amulet.armor; }
	[[nodiscard]] int CurrentHP() const { return HP; }
	[[nodiscard]] int CurrentMaxHP() const { return MaxHP + MaxHP * amulet.maxHpPercent / 100; }
	[[nodiscard]] int CurrentLevel() const { return level; }
	[[nodiscard]] double CurrentXP() const { return XP; }
	[[nodiscard]] std::optional<int> LevelUpMs() const { return level_up_ms; }
	[[nodiscard]] bool Alive() const { return HP > 0; }
	[[nodiscard]] float HealthRatio() const;
	// XP total at which the player reaches `lvl` (level 2 at 1000).
	[[nodiscard]] static double LevelXP(int lvl);
	[[nodiscard]] float LevelProgress() const; // 0..1 of the way from this level to the next
	[[nodiscard]] int RiddleXP() const;		   // a riddle answered: about a third of a level
	void AddMight(int ns = 1);
	void AddXP(int xp, WorldEvents& events); // a level up shows its status line and writes the levels note
	void Heal(int hpPart);					 // percent of max HP
	void HealFully() { HP = CurrentMaxHP(); }
	void AddArmor(int na = 1);
	// HP lost to a hit of dmg dealt as mix: the resistances, then the armour unless ignoreArmor; at least 1.
	[[nodiscard]] int HitDamage(int dmg, const DamageMix& mix, bool ignoreArmor) const;
	void LoseHP(int hp) { HP -= hp; }
	void AddMaxHP(int hpPart); // percent, heals fully
	// Applies a drunk potion's gain (ItemBag::Use took it out of the bag); returns the status line ("Healed 12
	// health").
	std::string Drink(const PotionGain& gain);
	// Puts on the worn amulet's bonus (a default AmuletBonus: none). keepShare: the HP keeps its share of the max HP
	// as the max changes (putting a health amulet on or off neither heals nor hurts); false after a load, whose HP is
	// already the worn one's.
	void Wear(const AmuletBonus& bonus, bool keepShare);
	[[nodiscard]] int PoisonResistPercent() const { return amulet.poisonResistPercent; }
	// The share of a spike or death trap's damage the amulet lets through; the hundredths left over carry to the
	// next hit, so the small spike hits are cut too (as Monster::StandInTrap).
	int TrapDamage(int dmg);
	// One update tick of the regeneration amulet: heals while `safe` (no monster can chase or attack the player) and
	// no poison runs.
	void Regenerate(bool safe, int tickMs);
	Poison poison;
	Resistances resist = NO_RESISTANCES; // how each type of a hit's damage is taken: the worn amulet's (Wear)
	void Dump(std::ofstream& f) const;
	void LoadDump(std::ifstream& f);
};

#endif
