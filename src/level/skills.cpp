#include "skills.h"
#include "campaign.h"
#include "texts.h"
#include <algorithm>
#include <bit>
#include <sstream>
#include <string>

const std::array<SkillDef, SKILL_COUNT> SKILLS = {{
	// Plan skill-stun: a blunt hit may stun.
	{"Stun", "Blunt hits may stun a foe", "Stuns nearly twice as often", false},
	{"Words of Thoth", "Read the hieroglyphs", nullptr, false},
	{"Dodge", "1 blow in 10 misses you", "About 1 blow in 5 misses you", false},
	// Plan skill-power-hit: a stamina skill, switched with the stamina-skill key.
	{"Power hit", "Held blows hit harder, for stamina", "Held blows hit 75% harder still", false},
	{"Eye of Horus", "Ranged weapons reach 30% farther; shoot up close", "Reach 60% farther; shoot point blank", false},
	{"Favour of Hapi", "Wade faster, jump out of water", nullptr, false},
	{"Serket's favour", "Poisons last half as long, 10% resisted", "Poisons last a third as long, 19% resisted", false},
	{"Blessing of Isis", "Curses last half as long, 10% resisted", "Curses last a third as long, 19% resisted", false},
	// Plan skill-flaming-arrows: a stamina skill, switched with the stamina-skill key.
	{"Flaming arrows", "Bow shots burn, for stamina", "Twice the fire, same stamina", false},
	// Plan heka-surplus: stats (PlayerStats), Seshat's exploration and the trapped containers.
	{"Ka of Osiris", "Max health +25%", "Max health +50%", false},
	{"Strength of Montu", "Might +5", "Might +10", false},
	{"Djed of Ptah", "Armor +5", "Armor +10", false},
	{"Breath of Shu", "Max stamina +25%", "Max stamina +50%", false},
	{"Speed of Mafdet", "Walk and sprint 20% faster", "Walk and sprint 35% faster", false},
	{"Wisdom of Seshat", "Hints, names, sealed rooms", nullptr, false},
	{"Disarm trap", "Chest traps fail, mimics shown", nullptr, false},
	// Plan skill-quiver-of-neith: more shots, back faster (PlayerStats::Shots, ShotRecoveryPace).
	{"Quiver of Neith",
	 "Ranged weapons hold 40% more shots, back 25% faster",
	 "70% more shots, back 44% faster",
	 false},
	// Plan skill-bleed: a cutting hit may make a foe bleed.
	{"Bleed", "Cutting hits may make a foe bleed", "Bleeds nearly twice as often", false},
}};

const char* skillLore(SkillId id) { return gameText("skill." + textSlug(skillDef(id).name) + ".lore").c_str(); }

const char* skillRankTwoLore(SkillId id) { return gameText("skill." + textSlug(skillDef(id).name) + ".rank2").c_str(); }

bool Heka::Earn(int level) {
	if (level == CAMPAIGN_LEVELS || std::ranges::contains(levels, level))
		return false;
	levels.push_back(level);
	return true;
}

int Heka::Learnt() const { return std::popcount(learned); }

int Heka::Spent() const { return std::popcount(learned) + std::popcount(upgraded); }

bool Heka::CanLearn(SkillId id) const { return !skillDef(id).sealed && !Has(id) && Free() > 0; }

bool Heka::CanUpgrade(SkillId id) const { return Has(id) && upgradable(id) && (upgraded & bit(id)) == 0 && Free() > 0; }

bool Heka::Waiting() const {
	if (Free() <= 0)
		return false;
	for (int i = 0; i < SKILL_COUNT; i++)
		if (CanLearn(static_cast<SkillId>(i)) || CanUpgrade(static_cast<SkillId>(i)))
			return true;
	return false;
}

bool Heka::Learn(SkillId id, bool force) {
	if (Has(id) || Free() <= 0 || (skillDef(id).sealed && !force))
		return false;
	learned |= bit(id);
	return true;
}

bool Heka::Upgrade(SkillId id) {
	if (!CanUpgrade(id))
		return false;
	upgraded |= bit(id);
	return true;
}

void Heka::Dump(std::ostream& out) const {
	out << "HEKA " << levels.size();
	for (int level : levels)
		out << ' ' << level;
	out << ' ' << learned << ' ' << upgraded << '\n';
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
	// The rest of the line: learnt, then upgraded (a save from before plan skill-upgrades has none).
	std::string rest;
	std::getline(in, rest);
	std::istringstream fields(rest);
	fields >> learned;
	fields >> upgraded;
	learned &= (1U << SKILL_COUNT) - 1;
	upgraded &= learned;
}
