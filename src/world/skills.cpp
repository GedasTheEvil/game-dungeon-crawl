#include "skills.h"
#include <algorithm>
#include <bit>
#include <string>

const std::array<SkillDef, SKILL_COUNT> SKILLS = {{
	{"Stun", "Blunt hits may stun a foe", "A good knock on the head stops anything for a moment, gods' beasts too.",
	 true},
	{"Words of Thoth", "Read the hieroglyphs", "Thoth, lord of writing, opens the carved words on the walls.", true},
	{"Dodge", "1 blow in 10 misses you", "Step aside at the last moment: a bite or a sting may find only air.", false},
	{"Power hit", "Harder blows for stamina", "Put your back into it: more damage a swing, paid in stamina.", true},
	{"Sharp eye", "Ranged weapons reach 30% farther", "Horus lends an eye: bows, sling and javelin carry farther.",
	 true},
	{"Favour of Hapi", "Wade faster, jump out of water", "Hapi, the Nile in flood, lets you through his shallows.",
	 true},
	{"Serket's favour", "Poisons last half as long", "Serket's children still bite, but they do not mean it as much.",
	 true},
	{"Blessing of Isis", "Curses last half as long", "Isis has a knot for every curse. She ties fast.", true},
}};

bool Heka::Earn(int level) {
	if (std::ranges::find(levels, level) != levels.end())
		return false;
	levels.push_back(level);
	return true;
}

int Heka::Spent() const { return std::popcount(learned); }

bool Heka::CanLearn(SkillId id) const { return !skillDef(id).sealed && !Has(id) && Free() > 0; }

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
