#ifndef SKILLS_H
#define SKILLS_H

#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
#include <vector>
#include <utility>

// Skills and heka, the points that buy them (plan skills): each boss killed gives one heka, the player spends it on a
// skill on the Heka screen. Skills are permanent, one heka each, no respec.

// Saved by place: append only, never reorder.
enum class SkillId : std::uint8_t {
	Stun,
	Hieroglyphs,
	Dodge,
	PowerHit,
	EyeOfHorus, // ranged reach, the minimum range shrinks (plan skill-eye-of-horus; was Sharp eye)
	FavourOfHapi,
	SerketsFavour,
	BlessingOfIsis,
	FlamingArrows,
	// Plan heka-surplus: more skills than heka, so each run picks a different set.
	KaOfOsiris, // max health +SKILL_HEALTH_PERCENT
	StrengthOfMontu, // Might +SKILL_MIGHT
	DjedOfPtah, // Armor +SKILL_ARMOR
	BreathOfShu, // max stamina +SKILL_STAMINA_PERCENT
	SpeedOfMafdet, // walk and sprint +SKILL_SPEED_PERCENT on firm ground, not the jump
	WisdomOfSeshat, // riddle hints, the royal name, hidden rooms on the map
	DisarmTrap, // a trapped container's trap may fail, trapped ones and mimics red-rimmed, never empty
	QuiverOfNeith, // ranged weapons hold QUIVER_SHOTS_PERCENT more shots, they come back QUIVER_RECOVERY_PERCENT faster
	Bleed, // plan skill-bleed: a cutting hit may make a foe bleed (Dungeon::bleedRoll)
};
constexpr int SKILL_COUNT = 18;

// The stat skills' gains (plan heka-surplus), applied where the stat is computed (PlayerStats).
constexpr int SKILL_HEALTH_PERCENT = 25;
constexpr int SKILL_MIGHT = 5;
constexpr int SKILL_ARMOR = 5;
constexpr int SKILL_STAMINA_PERCENT = 25;
constexpr int SKILL_SPEED_PERCENT = 20;
// Disarm trap: a trapped container's own roll to stay quiet, beside the trap ward's.
constexpr int DISARM_TRAP_PERCENT = 80;
// Quiver of Neith (plan skill-quiver-of-neith): added to the shots and shot recovery amulets' percents (PlayerStats).
constexpr int QUIVER_SHOTS_PERCENT = 40;
constexpr int QUIVER_RECOVERY_PERCENT = 25;

// Rank II (plan skill-upgrades): a second heka on a learnt skill adds 75%..125% of rank I. The values per rank are
// here; a chance's rank II gain is a share of the failure left (rankTwoChance), so nothing becomes sure.
constexpr int SKILL_HEALTH_PERCENT_2 = 50;
constexpr int SKILL_MIGHT_2 = 10;
constexpr int SKILL_ARMOR_2 = 10;
constexpr int SKILL_STAMINA_PERCENT_2 = 50;
constexpr int SKILL_SPEED_PERCENT_2 = 35; // 75%: movement and collision feel
constexpr int QUIVER_SHOTS_PERCENT_2 = 70; // 75%
constexpr int QUIVER_RECOVERY_PERCENT_2 = 44; // 75%
constexpr int FLAMING_ARROW_BONUS_PERCENT_2 = 80; // same stamina cost
constexpr int POWER_CURVE_RANK_2_PERCENT = 175; // 75%: rank II's bonus curve is this share of rank I's
constexpr int EYE_OF_HORUS_RANGE_PERCENT_2 = 160;
// Serket's favour and the Blessing of Isis: a poison or curse lasts 1 / divisor as long.
constexpr int GOD_SKILL_DIVISOR = 2;
constexpr int GOD_SKILL_DIVISOR_2 = 3;
// Rank II of a chance (Dodge, Stun, Bleed, the god resists): rank I's chance, plus share % of rank I's chance taken
// from what still fails. 10% at a share of 100: 19%.
[[nodiscard]] constexpr int rankTwoChance(int chance, int share = 100) {
	return chance + chance * share / 100 * (100 - chance) / 100;
}

struct SkillDef {
	const char* name;
	const char* effect; // one line, on the card
	// Rank II's effect line (plan skill-upgrades); nullptr: no rank II.
	const char* rankTwo;
	// Its rules are not in the game yet (its sub-draft in docs/plan/): shown, not learnable. A sub-draft that lands
	// clears it.
	bool sealed;
};
extern const std::array<SkillDef, SKILL_COUNT> SKILLS;
[[nodiscard]] inline const SkillDef& skillDef(SkillId id) { return SKILLS[std::to_underlying(id)]; }
// The details, under the effect on its card (content/texts/skills.txt).
[[nodiscard]] const char* skillLore(SkillId id);
// Rank II's details (content/texts/skills.txt, `rank2`); only for an upgradable skill.
[[nodiscard]] const char* skillRankTwoLore(SkillId id);
[[nodiscard]] inline bool upgradable(SkillId id) { return skillDef(id).rankTwo != nullptr; }

// Cells of media/textures/ui/skill_icons.png (tools/textures/skill_icons.py): the heka mark, then the skills in SkillId
// order.
constexpr int HEKA_ICON = 0;
constexpr int SKILL_ICON_FIRST = 1;
constexpr int SKILL_ICON_COLUMNS = 4;
constexpr int SKILL_ICON_ROWS = (SKILL_ICON_FIRST + SKILL_COUNT + SKILL_ICON_COLUMNS - 1) / SKILL_ICON_COLUMNS;
[[nodiscard]] constexpr int skillIcon(SkillId id) { return SKILL_ICON_FIRST + static_cast<int>(id); }

// The player's heka: the bosses that gave one (by campaign level, so a boss killed again after a load gives none)
// and the skills learnt, rank I or II. Saved with PlayerStats.
class Heka {
  public:
	// The boss of `level` is slain: one heka, the first time, none for the last level's (Anubis: the win is the
	// reward, plan heka-surplus). True when it gave one.
	bool Earn(int level);
	[[nodiscard]] int Earned() const { return static_cast<int>(levels.size()); }
	[[nodiscard]] int Free() const { return Earned() - Spent(); }
	[[nodiscard]] int Spent() const; // learnt skills and rank IIs
	[[nodiscard]] int Learnt() const; // skills learnt, whatever the rank
	[[nodiscard]] bool Has(SkillId id) const { return (learned & bit(id)) != 0; }
	// 0 not learnt, 1 learnt, 2 upgraded (plan skill-upgrades).
	[[nodiscard]] int Rank(SkillId id) const { return Has(id) ? ((upgraded & bit(id)) != 0 ? 2 : 1) : 0; }
	// Rank I's value, rank II's, or none.
	template <typename T> [[nodiscard]] T ByRank(SkillId id, T one, T two, T none = T{}) const {
		const int rank = Rank(id);
		return rank == 2 ? two : (rank == 1 ? one : none);
	}
	// One that the stamina-skill toggle switches: flaming arrows, power hit.
	[[nodiscard]] bool HasStaminaSkill() const { return Has(SkillId::FlamingArrows) || Has(SkillId::PowerHit); }
	// Not learnt yet, not sealed, and a heka free.
	[[nodiscard]] bool CanLearn(SkillId id) const;
	// Learnt, upgradable, not upgraded yet, and a heka free.
	[[nodiscard]] bool CanUpgrade(SkillId id) const;
	// A heka free and a skill to learn or upgrade (the HUD tile pulses).
	[[nodiscard]] bool Waiting() const;
	// Spends a heka on it; false (nothing spent) unless CanLearn. Force: also a sealed one (scenario `learn`).
	bool Learn(SkillId id, bool force = false);
	// Spends a heka on rank II; false (nothing spent) unless CanUpgrade.
	bool Upgrade(SkillId id);

	// "HEKA n level... learnt upgraded"; LoadDump reads it when the tag is there (saves from before the skills have none).
	void Dump(std::ostream& out) const;
	void LoadDump(std::istream& in);

  private:
	std::vector<int> levels; // campaign levels whose boss gave a heka
	unsigned learned = 0; // bit per SkillId
	unsigned upgraded = 0; // bit per SkillId: rank II, a subset of learned

	static constexpr unsigned bit(SkillId id) { return 1U << static_cast<unsigned>(id); }
};

#endif
