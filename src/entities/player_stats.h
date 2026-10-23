#ifndef PLAYER_STATS_H
#define PLAYER_STATS_H

#include "../core/timer.h"
#include "../world/curses.h"
#include "../world/damage.h"
#include "../world/inscriptions.h"
#include "../world/item_bag.h"
#include "../world/poison.h"
#include "../world/run_stats.h"
#include "../world/skills.h"
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
	int max_stamina_percent = 0; // inscription boons' (plan inscription-boons), on top of the level's max
	AmuletBonus amulet; // the worn amulet's (Wear); Armor, MaxHP and Might above are without it
	int regen_carry_ms = 0; // regeneration time not yet turned into HP
	int trap_carry = 0; // hundredths of trap damage not yet taken (TrapDamage)
	int resist_percent = 0; // the last resistance potion's, while resist_left_ms runs
	int resist_left_ms = 0;
	int water_left_ms = 0; // the Blessing of Hapi's time left (plan water-blessing-potion)

	Timer stamina_regen_timer{1000};
	Timer stamina_sprint_drain_timer{1000};
	float stamina_regen_carry = 0.f;
	float stamina_sprint_drain_carry = 0.f;
	bool sprint_requested = false; // shift held
	bool walked = false; // the player took a walk step this tick (NoteWalked)
	bool walked_cheap = false; // ... a wade step that drains the sprint's stamina at half (NoteWalked)
	bool sprinting = false; // shift held and the player moved last tick: stamina drains
	std::optional<int> level_up_ms; // game clock time of the last level up, for the sun beam
	std::optional<int> stamina_refused_ms; // last jump or sprint refused for lack of stamina, for the HUD flash

	bool AdvanceLevel(WorldEvents& events);
	void RegenerateStamina();
	void keepHpShare(int oldMax); // the HP keeps its share of the max HP as the max changed from oldMax

  public:
	void SetSprintRequested(bool requested) { sprint_requested = requested; }
	[[nodiscard]] bool IsSprinting() const { return sprinting; }
	// Shift held with stamina left: the step is already a sprint step, before UpdateStamina sees it moved. Slowness
	// sprints less (SLOWNESS_SPRINT_MULTIPLIER).
	[[nodiscard]] float SprintMoveMultiplier() const;
	// Walk and climb steps: 1, SLOWNESS_WALK_FACTOR while slowed.
	[[nodiscard]] float WalkCurseFactor() const;
	// A walk step really moved the player (not into a wall). halvedDrain: it was a sprint in half water with the
	// Blessing of Hapi and the Favour of Hapi, which drains the stamina at WADE_SPRINT_DRAIN_BOTH.
	void NoteWalked(bool halvedDrain = false) {
		walked = true;
		walked_cheap = halvedDrain;
	}
	void UpdateStamina(WorldEvents& events);
	[[nodiscard]] int Stamina() const { return stamina; }
	[[nodiscard]] int MaxStamina() const { return (100 + (level - 1) * 10) * (100 + max_stamina_percent) / 100; }
	void SetStamina(int value);
	bool ConsumeStamina(int value); // false (and nothing spent) if there is not enough
	void AddStamina(int value);
	[[nodiscard]] float StaminaRatio() const;
	void RefuseStamina(WorldEvents& events); // a jump or sprint wanted more stamina than there is
	[[nodiscard]] std::optional<int> StaminaRefusedMs() const { return stamina_refused_ms; }

	// Might and weapon, cut by clumsiness; at least 1.
	[[nodiscard]] int Damage(int weaponDamage) const { return cursedHit(CurrentMight() + weaponDamage, curses); }
	// With the worn amulet's bonus and the running curses (weakness, vulnerability, disease).
	[[nodiscard]] int CurrentMight() const { return cursedMight(Might + amulet.might, curses); }
	[[nodiscard]] int CurrentArmor() const { return cursedArmor(Armor + amulet.armor, curses); }
	[[nodiscard]] int CurrentHP() const { return HP; }
	[[nodiscard]] int CurrentMaxHP() const { return cursedMaxHP(MaxHP + MaxHP * amulet.maxHpPercent / 100, curses); }
	[[nodiscard]] int CurrentLevel() const { return level; }
	[[nodiscard]] double CurrentXP() const { return XP; }
	[[nodiscard]] std::optional<int> LevelUpMs() const { return level_up_ms; }
	[[nodiscard]] bool Alive() const { return HP > 0; }
	[[nodiscard]] float HealthRatio() const;
	// XP total at which the player reaches `lvl` (level 2 at 1000).
	[[nodiscard]] static double LevelXP(int lvl);
	[[nodiscard]] float LevelProgress() const; // 0..1 of the way from this level to the next
	[[nodiscard]] int RiddleXP() const; // a riddle answered: about a third of a level
	[[nodiscard]] int InscriptionXP() const; // an inscription translated: a part of a riddle's reward
	void AddMight(int ns = 1);
	void AddXP(int xp, WorldEvents& events); // a level up shows its status line and writes the levels note
	void Heal(int hpPart); // percent of max HP
	void HealFully() { HP = CurrentMaxHP(); }
	void AddArmor(int na = 1);
	// HP lost to a hit of dmg dealt as mix: the resistances, then the armour unless ignoreArmor; at least 1.
	[[nodiscard]] int HitDamage(int dmg, const DamageMix& mix, bool ignoreArmor, bool inWater = false) const;
	void LoseHP(int hp) { HP -= hp; }
	void AddMaxHP(int hpPart); // percent, heals fully
	void AddMaxStamina(int percent); // the stamina rises by as much as the max
	// An inscription's boon, given when it is translated; returns its text ("Might +1"), empty for none.
	std::string Grant(const Boon& boon);
	// Applies a drunk potion's gain (ItemBag::Use took it out of the bag); returns the status line ("Healed 12
	// health").
	std::string Drink(const PotionGain& gain);
	// Puts on the worn amulet's bonus (a default AmuletBonus: none). keepShare: the HP keeps its share of the max HP
	// as the max changes (putting a health amulet on or off neither heals nor hurts); false after a load, whose HP is
	// already the worn one's.
	void Wear(const AmuletBonus& bonus, bool keepShare);
	// The chance a poisoned hit does not poison: the amulet's and a running resistance potion's added up, at most 100.
	[[nodiscard]] int PoisonResistPercent() const;
	[[nodiscard]] int PotionResistPercent() const { return resist_left_ms > 0 ? resist_percent : 0; }
	[[nodiscard]] int PotionResistLeftMs() const { return resist_left_ms; }
	void AdvanceResistance(int ms); // the potion's time runs out
	void EndResistance() { resist_left_ms = 0; }
	// The Blessing of Hapi potion (plan water-blessing-potion): lifts the water's limits while it lasts.
	[[nodiscard]] bool WaterBlessed() const { return water_left_ms > 0; }
	[[nodiscard]] int WaterBlessingLeftMs() const { return water_left_ms; }
	// The potion's time runs out; the status box says when it ends.
	void AdvanceWaterBlessing(int ms, WorldEvents& events);
	void EndWaterBlessing() { water_left_ms = 0; }
	// The venom amulet: the chance a weapon hit poisons the monster, and with which tier.
	[[nodiscard]] int VenomPercent() const { return amulet.venomPercent; }
	[[nodiscard]] PoisonTier VenomTier() const { return amulet.venomTier; }
	// The trap ward's cut of trap damage, and the chance a chest's trap or a tomb curse does not go off (plan
	// trapped-chests); 100: immune.
	[[nodiscard]] int TrapWardPercent() const { return amulet.trapCutPercent; }
	// The share of a spike or death trap's damage the amulet lets through; the hundredths left over carry to the
	// next hit, so the small spike hits are cut too (as Monster::StandInTrap).
	int TrapDamage(int dmg);
	// One update tick of the regeneration amulet: heals while `safe` (no monster can chase or attack the player) and
	// no poison runs.
	void Regenerate(bool safe, int tickMs);
	// Lays a curse (plan curses): its status line, the field note the first time. Disease keeps the HP's share of the
	// max HP; fatigue drops the stamina to FATIGUE_STAMINA_PERCENT. Returns true.
	bool LayCurse(Curse c, CurseGrade grade, WorldEvents& events);
	void AdvanceCurses(int ms, WorldEvents& events); // the curses' time runs out, each with its status line
	void EndCurses(); // death or a new game: all gone
	Curses curses;
	Poison poison;
	Resistances resist = NO_RESISTANCES; // how each type of a hit's damage is taken: the worn amulet's (Wear)
	Heka heka; // bosses' points and the skills bought with them
	RunStats run; // what this run did since New Game, for the leaderboard (plan leaderboard)
	void Dump(std::ofstream& f) const;
	void LoadDump(std::ifstream& f);
};

#endif
