#ifndef SKILLS_H
#define SKILLS_H

#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
#include <vector>

// Skills and heka, the points that buy them (plan skills): each boss killed gives one heka, the player spends it on a
// skill on the Heka screen. Skills are permanent, one heka each, no respec.

// Saved by place: append only, never reorder.
enum class SkillId : std::uint8_t {
	Stun,
	Hieroglyphs,
	Dodge,
	PowerHit,
	SharpEye,
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
};
constexpr int SKILL_COUNT = 16;

// The stat skills' gains (plan heka-surplus), applied where the stat is computed (PlayerStats).
constexpr int SKILL_HEALTH_PERCENT = 25;
constexpr int SKILL_MIGHT = 5;
constexpr int SKILL_ARMOR = 5;
constexpr int SKILL_STAMINA_PERCENT = 25;
constexpr int SKILL_SPEED_PERCENT = 20;
// Disarm trap: a trapped container's own roll to stay quiet, beside the trap ward's.
constexpr int DISARM_TRAP_PERCENT = 80;

struct SkillDef {
	const char* name;
	const char* effect; // one line, on the card
	// Its rules are not in the game yet (its sub-draft in docs/plan/): shown, not learnable. A sub-draft that lands
	// clears it.
	bool sealed;
};
extern const std::array<SkillDef, SKILL_COUNT> SKILLS;
[[nodiscard]] inline const SkillDef& skillDef(SkillId id) { return SKILLS[static_cast<size_t>(id)]; }
// The details, under the effect on its card (content/texts/skills.txt).
[[nodiscard]] const char* skillLore(SkillId id);

// Cells of media/textures/ui/skill_icons.png (tools/textures/skill_icons.py): the heka mark, then the skills in SkillId
// order.
constexpr int HEKA_ICON = 0;
constexpr int SKILL_ICON_FIRST = 1;
constexpr int SKILL_ICON_COLUMNS = 4;
constexpr int SKILL_ICON_ROWS = (SKILL_ICON_FIRST + SKILL_COUNT + SKILL_ICON_COLUMNS - 1) / SKILL_ICON_COLUMNS;
[[nodiscard]] constexpr int skillIcon(SkillId id) { return SKILL_ICON_FIRST + static_cast<int>(id); }

// The player's heka: the bosses that gave one (by campaign level, so a boss killed again after a load gives none)
// and the skills learnt. Saved with PlayerStats.
class Heka {
  public:
	// The boss of `level` is slain: one heka, the first time, none for the last level's (Anubis: the win is the
	// reward, plan heka-surplus). True when it gave one.
	bool Earn(int level);
	[[nodiscard]] int Earned() const { return static_cast<int>(levels.size()); }
	[[nodiscard]] int Free() const { return Earned() - Spent(); }
	[[nodiscard]] int Spent() const;
	[[nodiscard]] bool Has(SkillId id) const { return (learned & bit(id)) != 0; }
	// One that the stamina-skill toggle switches: flaming arrows, power hit.
	[[nodiscard]] bool HasStaminaSkill() const { return Has(SkillId::FlamingArrows) || Has(SkillId::PowerHit); }
	// Not learnt yet, not sealed, and a heka free.
	[[nodiscard]] bool CanLearn(SkillId id) const;
	// A heka free and a skill to spend it on (the HUD tile pulses).
	[[nodiscard]] bool Waiting() const;
	// Spends a heka on it; false (nothing spent) unless CanLearn. Force: also a sealed one (scenario `learn`).
	bool Learn(SkillId id, bool force = false);

	// "HEKA n level... learnt"; LoadDump reads it when the tag is there (saves from before the skills have none).
	void Dump(std::ostream& out) const;
	void LoadDump(std::istream& in);

  private:
	std::vector<int> levels; // campaign levels whose boss gave a heka
	unsigned learned = 0; // bit per SkillId

	static constexpr unsigned bit(SkillId id) { return 1U << static_cast<unsigned>(id); }
};

#endif
