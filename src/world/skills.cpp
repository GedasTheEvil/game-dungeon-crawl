#include "skills.h"
#include "campaign.h"
#include "texts.h"
#include <algorithm>
#include <bit>
#include <string>

const std::array<SkillDef, SKILL_COUNT> SKILLS = {{
	// Plan skill-stun: a blunt hit may stun.
	{"Stun", "Blunt hits may stun a foe", false},
	{"Words of Thoth", "Read the hieroglyphs", false},
	{"Dodge", "1 blow in 10 misses you", false},
	// Plan skill-power-hit: a stamina skill, switched with the stamina-skill key.
	{"Power hit", "Held blows hit harder, for stamina", false},
	{"Sharp eye", "Ranged weapons reach 30% farther", false},
	{"Favour of Hapi", "Wade faster, jump out of water", false},
	{"Serket's favour", "Poisons last half as long", false},
	{"Blessing of Isis", "Curses last half as long", false},
	// Plan skill-flaming-arrows: a stamina skill, switched with the stamina-skill key.
	{"Flaming arrows", "Bow shots burn, for stamina", false},
	// Plan heka-surplus: stats (PlayerStats), Seshat's exploration and the trapped containers.
	{"Ka of Osiris", "Max health +25%", false},
	{"Strength of Montu", "Might +5", false},
	{"Djed of Ptah", "Armor +5", false},
	{"Breath of Shu", "Max stamina +25%", false},
	{"Speed of Mafdet", "Walk and sprint 20% faster", false},
	{"Wisdom of Seshat", "Hints, names, rooms, traps", false},
	{"Disarm trap", "Chest traps fail, mimics glow", false},
}};

const char* skillLore(SkillId id) { return gameText("skill." + textSlug(skillDef(id).name) + ".lore").c_str(); }

bool Heka::Earn(int level) {
	if (level == CAMPAIGN_LEVELS || std::ranges::find(levels, level) != levels.end())
		return false;
	levels.push_back(level);
	return true;
}

int Heka::Spent() const { return std::popcount(learned); }

bool Heka::CanLearn(SkillId id) const { return !skillDef(id).sealed && !Has(id) && Free() > 0; }

bool Heka::Waiting() const {
	if (Free() <= 0)
		return false;
	for (int i = 0; i < SKILL_COUNT; i++)
		if (CanLearn(static_cast<SkillId>(i)))
			return true;
	return false;
}

bool Heka::Learn(SkillId id, bool force) {
	if (Has(id) || Free() <= 0 || (skillDef(id).sealed && !force))
		return false;
	learned |= bit(id);
	return true;
}

void Heka::Dump(std::ostream& out) const {
	out << "HEKA " << levels.size();
	for (int level : levels)
		out << ' ' << level;
	out << ' ' << learned << '\n';
}

void Heka::LoadDump(std::istream& in) {
	*this = Heka{};
	const auto start = in.tellg();
	std::string tag;
	size_t count = 0;
	if (!(in >> tag) || tag != "HEKA" || !(in >> count)) {
		in.clear();
		in.seekg(start);
		return;
	}
	for (size_t i = 0; i < count; i++) {
		int level = 0;
		if (!(in >> level))
			return;
		levels.push_back(level);
	}
	in >> learned;
	learned &= (1U << SKILL_COUNT) - 1;
}
