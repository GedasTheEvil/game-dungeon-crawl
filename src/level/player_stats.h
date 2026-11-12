#ifndef PLAYER_STATS_H
#define PLAYER_STATS_H

#include "gameplay_config.h"
#include "../core/timer.h"
#include "curses.h"
#include "damage.h"
#include "inscriptions.h"
#include "item_bag.h"
#include "poison.h"
#include "vitals.h"
#include "run_stats.h"
#include "shrines.h"
#include "terrain.h"
#include "skills.h"
#include "world_events.h"
#include <array>
#include <fstream>
#include <optional>
#include <string>

constexpr int HP_PER_LEVEL = 12; // max HP gained per level up

// What learning a skill cured (plan learn-skill-cures).
enum class LearnCure { None, Poison, Curses };

// The player's level, XP, might, armour, HP and stamina (sprint drains it, it regenerates). Saved with the game.
class PlayerStats {
  private:
	int level = 1;
	double XP = 0;
	int Armor = 0;
	int MaxHP = 50;
	int Might = 0;
	int stamina = 100;
	int max_stamina_percent = 0; // inscription boons' (plan inscription-boons), on top of the level's max
	AmuletBonus amulet; // the worn amulet's (Wear); Armor, MaxHP and Might above are without it
	int regen_carry_ms = 0; // regeneration time not yet turned into HP
	RingBonus rings; // the worn rings' (WearRings)
	int ring_regen_carry = 0; // the garnets' ms x HP an hour not yet turned into HP
	int resist_percent = 0; // the last resistance potion's, while resist_left_ms runs
	int resist_left_ms = 0;
	int water_left_ms = 0; // the Blessing of Hapi's time left (plan water-blessing-potion)
	// Each god's blessing's time left, by God (plans shrine-of-isis, shrines-of-the-gods); Isis's is her ward.
	std::array<int, GOD_COUNT> blessing_left_ms{};

	Timer stamina_regen_timer{1000};
	Timer stamina_sprint_drain_timer{1000};
	float stamina_regen_carry = 0.f;
	float stamina_sprint_drain_carry = 0.f;
	bool sprint_requested = false; // shift held
	bool walked = false; // the player took a walk step this tick (NoteWalked)
	bool walked_cheap = false; // ... a wade step that drains the sprint's stamina at half (NoteWalked)
	bool sprinting = false; // shift held and the player moved last tick: stamina drains
	bool charging = false; // a power hit is being charged (Player::StartCharge): no regen, no sprint
	std::optional<int> level_up_ms; // game clock time of the last level up, for the sun beam
	std::optional<int>
		prayed_ms; // ... of the last prayer at a shrine, for its beam in the god's tint (plan shrine-of-isis)
	God prayed_god = God::Isis; // ... and to whom
	std::optional<int> stamina_refused_ms; // last jump or sprint refused for lack of stamina, for the HUD flash

	bool AdvanceLevel(WorldEvents& events);
	void RegenerateStamina();
	void keepHpShare(int oldMax); // the HP keeps its share of the max HP as the max changed from oldMax

  public:
	void SetSprintRequested(bool requested) { sprint_requested = requested; }
	// A power hit's charge holds the stamina (plan skill-power-hit): it does not regenerate, shift does not sprint.
	void SetCharging(bool on) { charging = on; }
	[[nodiscard]] bool IsSprinting() const { return sprinting; }
	// Shift held with stamina left: the step is already a sprint step, before UpdateStamina sees it moved. Slowness
	// sprints less (SLOWNESS_SPRINT_MULTIPLIER).
	[[nodiscard]] float SprintMoveMultiplier() const;
	// Walk and climb steps: SLOWNESS_WALK_FACTOR while slowed, times the jasper rings' speed on the ground (not in the
	// air: a jump carries as far as without them, the levels are built on its reach). The Speed of Mafdet skill too,
	// and only on firm ground (not wading, not in sand: their own factors stay as they are; plan heka-surplus).
	[[nodiscard]] float WalkFactor(bool airborne, bool firmGround = true) const;
	// A walk step really moved the player (not into a wall). halvedDrain: it was a sprint in half water with the
	// Blessing of Hapi and the Favour of Hapi, which drains the stamina at WADE_SPRINT_DRAIN_BOTH.
	void NoteWalked(bool halvedDrain = false) {
		walked = true;
		walked_cheap = halvedDrain;
	}
	void UpdateStamina(WorldEvents& events);
	[[nodiscard]] int Stamina() const { return stamina; }
	[[nodiscard]] int MaxStamina() const {
		const int skill = heka.Has(SkillId::BreathOfShu) ? SKILL_STAMINA_PERCENT : 0;
		return (100 + (level - 1) * 10) * (100 + max_stamina_percent + skill) / 100;
	}
	void SetStamina(int value);
	bool ConsumeStamina(int value); // false (and nothing spent) if there is not enough
	void AddStamina(int value);
	[[nodiscard]] float StaminaRatio() const;
	void RefuseStamina(WorldEvents& events); // a jump or sprint wanted more stamina than there is
	[[nodiscard]] std::optional<int> StaminaRefusedMs() const { return stamina_refused_ms; }

	// Might and weapon, cut by clumsiness; at least 1.
	[[nodiscard]] int Damage(int weaponDamage) const { return cursedHit(CurrentMight() + weaponDamage, curses); }
	// With the worn amulet's bonus, the stat skills' (plan heka-surplus) and the running curses (weakness,
	// vulnerability, disease).
	[[nodiscard]] int CurrentMight() const {
		const int skill = heka.Has(SkillId::StrengthOfMontu) ? SKILL_MIGHT : 0;
		return cursedMight(Might + amulet.might + skill + (Blessed(God::Montu) ? MONTU_MIGHT : 0), curses);
	}
	[[nodiscard]] int CurrentArmor() const {
		const int skill = heka.Has(SkillId::DjedOfPtah) ? SKILL_ARMOR : 0;
		return cursedArmor(Armor + amulet.armor + skill + (Blessed(God::Ptah) ? PTAH_ARMOR : 0), curses);
	}
	[[nodiscard]] int CurrentHP() const { return vitals.hp; }
	[[nodiscard]] int CurrentMaxHP() const {
		const int percent = amulet.maxHpPercent + (heka.Has(SkillId::KaOfOsiris) ? SKILL_HEALTH_PERCENT : 0) +
							(Blessed(God::Amun) ? AMUN_MAX_HP_PERCENT : 0);
		return cursedMaxHP(MaxHP + MaxHP * percent / 100, curses);
	}
	// Spends a heka on the skill (Heka::Learn); a health or stamina skill gives the new share of the max on top (no
	// heal beyond it). False: nothing learnt. Serket's favour cures a running poison and the Blessing of Isis lifts
	// every curse, once (plan learn-skill-cures); `cure` tells which one did something.
	bool Learn(SkillId id, bool force = false, LearnCure* cure = nullptr);
	[[nodiscard]] int CurrentLevel() const { return level; }
	[[nodiscard]] double CurrentXP() const { return XP; }
	[[nodiscard]] std::optional<int> LevelUpMs() const { return level_up_ms; }
	[[nodiscard]] std::optional<int> PrayedMs() const { return prayed_ms; }
	[[nodiscard]] God PrayedGod() const { return prayed_god; }
	[[nodiscard]] bool Alive() const { return vitals.Alive(); }
	[[nodiscard]] float HealthRatio() const;
	// XP total at which the player reaches `lvl` (level 2 at 1000).
	[[nodiscard]] static double LevelXP(int lvl);
	[[nodiscard]] float LevelProgress() const; // 0..1 of the way from this level to the next
	[[nodiscard]] int RiddleXP() const; // a riddle answered: about a third of a level
	[[nodiscard]] int InscriptionXP() const; // an inscription translated: a part of a riddle's reward
	void AddMight(int ns = 1);
	void AddXP(int xp, WorldEvents& events); // a level up shows its status line and writes the levels note
	void Heal(int hpPart); // percent of max HP
	void HealFully() { vitals.hp = CurrentMaxHP(); }
	void AddArmor(int na = 1);
	// HP lost to a hit of dmg dealt as mix: the resistances, then the armour unless ignoreArmor; at least 1.
	[[nodiscard]] int HitDamage(int dmg, const DamageMix& mix, bool ignoreArmor, bool inWater = false) const;
	// How the player takes a hit (Vitals::Cost): the worn resistances and the current armour.
	[[nodiscard]] Guard HitGuard() const { return {resist, CurrentArmor()}; }
	void LoseHP(int hp) { vitals.Lose(hp); }
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
	[[nodiscard]] int AmuletPoisonResistPercent() const { return amulet.poisonResistPercent; }
	[[nodiscard]] int PoisonResistPercent() const; // amulet + potion + Serket's favour 10%, max 100
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
	// The gods' blessings (plans shrine-of-isis, shrines-of-the-gods, shrines.h): prayed for at the god's shrine.
	[[nodiscard]] bool Blessed(God g) const { return BlessingLeftMs(g) > 0; }
	[[nodiscard]] int BlessingLeftMs(God g) const { return blessing_left_ms[static_cast<size_t>(g)]; }
	// The Protection of Isis: no curse lands while it lasts.
	[[nodiscard]] bool IsisWarded() const { return Blessed(God::Isis); }
	[[nodiscard]] int IsisWardLeftMs() const { return BlessingLeftMs(God::Isis); }
	// Prays at the god's shrine and (re)starts the blessing. Isis: every curse lifted, her ward for ISIS_WARD_MS
	// (twice with the Blessing of Isis skill). Montu: Might +MONTU_MIGHT. Ptah: Armor +PTAH_ARMOR. Amun: max HP
	// +AMUN_MAX_HP_PERCENT, healed to it. Serket: the poison cured, no poison lands (twice as long with Serket's
	// favour). BLESSING_MS each. Its status line, the sound and the field note.
	void Pray(God g, WorldEvents& events);
	// An offering at the god's shrine (plan shrine-offerings): only the prayer's beam in the god's tint.
	void Offered(God g);
	// The blessings' time runs out; the status box says when one ends. Amun's end keeps the HP within the max, >= 1.
	void AdvanceBlessings(int ms, WorldEvents& events);
	void EndBlessings(); // death or a new game: all gone
	// The venom amulet: the chance a weapon hit poisons the monster, and with which tier.
	[[nodiscard]] int VenomPercent() const { return amulet.venomPercent; }
	[[nodiscard]] PoisonTier VenomTier() const { return amulet.venomTier; }
	// The crushing, cleaving and piercing amulets (plan amulet-extras): the percent more a weapon hit of this damage
	// mix deals, only the matching shares of the mix grow (sword 85% slash, grand Cleaving +50%: +42%).
	[[nodiscard]] int TypeMightPercent(const DamageMix& mix) const;
	// The trap ward's cut of trap damage, and the chance a chest's trap or a tomb curse does not go off (plan
	// trapped-chests); 100: immune.
	[[nodiscard]] int TrapWardPercent() const { return amulet.trapCutPercent; }
	// Its terrain affinity (plan same-hazards-for-all), as a monster kind's native and hostile terrains: the hook a
	// blessing or a curse adds a terrain through. Today none native, webs hostile (PLAYER_HOSTILE_TERRAINS).
	[[nodiscard]] Terrains TerrainNative() const { return {}; }
	[[nodiscard]] Terrains TerrainHostile() const { return PLAYER_HOSTILE_TERRAINS; }
	// The share of a trap's damage it takes, per-mille (Vitals::TrapLoss): all but the ward's.
	[[nodiscard]] int TrapPermille() const { return FULL_TRAP_SHARE - 10 * amulet.trapCutPercent; }
	// A trap's damage in whole HP, of it the share the amulet lets through (TrapPermille); the fraction left over
	// carries to the next hit, so the small spike hits are cut too (Vitals::TrapLoss, as a monster's).
	int TrapDamage(int dmg);
	// The same for a damage in hundredths of a HP (a crossing's bell, a jet's tick).
	int TrapDamageHundredths(int hundredths) { return vitals.TrapLoss(hundredths, TrapPermille()); }
	// One update tick of the regeneration amulet: heals while `safe` (no monster can chase or attack the player) and
	// no poison runs. The rubies heal too, also when not safe; not while poisoned either.
	void Regenerate(bool safe, int tickMs);
	// Puts on the worn rings' bonus (a default RingBonus: none).
	void WearRings(const RingBonus& bonus);
	[[nodiscard]] const RingBonus& Rings() const { return rings; }
	// The feldspar rings: the poison's time runs down this much faster.
	[[nodiscard]] int PoisonFasterPercent() const { return rings.poisonFasterPercent; }
	// Tiles: the weapon's reach with the Sharp eye skill (weaponReach) and the rock crystal rings.
	[[nodiscard]] float WeaponReach(ItemKind weapon) const {
		return weaponReach(weapon, heka) * static_cast<float>(100 + rings.reachPercent) / 100.f;
	}
	// A ranged weapon's shots and how they come back (plan ranged-weaker-than-melee): WeaponDef::shots,
	// SHOT_RECOVERY_DELAY_MS and SHOT_RECOVERY_MS. The one place to scale them: the shots amulet adds its share of the
	// shots, rounded up, at least one (plan amulet-more-shots); the quick shots amulet shortens both times, +100%
	// halves them (plan amulet-quick-shots). The Quiver of Neith skill adds its percents to both (plan
	// skill-quiver-of-neith).
	[[nodiscard]] int Shots(ItemKind weapon) const;
	[[nodiscard]] ShotPace ShotRecoveryPace() const;
	// Lays a curse (plan curses): its status line, the field note the first time. Disease keeps the HP's share of the
	// max HP; fatigue drops the stamina to FATIGUE_STAMINA_PERCENT. Returns true; false when the Protection of Isis
	// wards it off (a line of its own, no curse, not counted).
	// Plan skill-god-resist: the Isis ward turns it away, then CurseResistPercent is rolled on rng. True: it landed.
	bool LayCurse(Curse c, CurseGrade grade, WorldEvents& events, Rng& rng);
	// The curse itself, no ward and no resist roll (the scenario `curse` command).
	void TakeCurse(Curse c, CurseGrade grade, WorldEvents& events);
	// The chance a curse does not land: Blessing of Isis skill 10% (plan skill-god-resist), future sources add; max
	// 100.
	[[nodiscard]] int CurseResistPercent() const;
	void AdvanceCurses(int ms, WorldEvents& events); // the curses' time runs out, each with its status line
	void EndCurses(); // death or a new game: all gone
	Curses curses;
	// HP, poison, fire, water, the trap ramp, the daze (plan shared-vitals); the HP and the poison are saved.
	Vitals vitals{50};
	Resistances resist = NO_RESISTANCES; // how each type of a hit's damage is taken: the worn amulet's (Wear)
	Heka heka; // bosses' points and the skills bought with them
	RunStats run; // what this run did since New Game, for the leaderboard (plan leaderboard)
	void Dump(std::ostream& f) const;
	void LoadDump(std::istream& f);
};

#endif
