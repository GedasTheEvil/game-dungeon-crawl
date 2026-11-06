#include "curses.h"
#include <algorithm>
#include <string>

const std::array<CurseDef, CURSE_COUNT> CURSES = {{
	{"Weakness", "Greater weakness", "A curse! You feel weak", "The weakness grips you again", "The weakness passes"},
	{"Clumsiness",
	 "Greater clumsiness",
	 "A curse! Your blows go astray",
	 "Your hands grow clumsier",
	 "Your hands are your own again"},
	{"Vulnerability",
	 nullptr,
	 "A curse! Your armour feels like papyrus",
	 "Your armour weakens again",
	 "Your armour holds again"},
	{"Fatigue", nullptr, "A curse! Your strength drains away", "The fatigue deepens", "The fatigue lifts"},
	{"Disease", "Greater disease", "A curse! A sickness takes you", "The sickness flares up", "The sickness passes"},
	{"Slowness", nullptr, "A curse! Your legs grow heavy", "Your legs grow heavy again", "Your legs are light again"},
	{"Palsy", nullptr, "A curse! Your hands shake", "Your hands shake again", "Your hands are steady again"},
	{"Insanity", nullptr, "A curse! Your mind is clouded", "Your mind clouds again", "Your mind clears"},
}};

namespace {
int cut(int value, int percent) { return value * (100 - percent) / 100; }
} // namespace

bool Curses::Lay(Curse c, CurseGrade grade) { return Lay(c, grade, CURSE_MS); }

bool Curses::Lay(Curse c, CurseGrade grade, int durationMs) {
	Running& r = running[static_cast<size_t>(c)];
	const bool fresh = r.leftMs <= 0;
	if (fresh || grade == CurseGrade::Greater)
		r.grade = grade;
	r.leftMs = std::clamp(durationMs, 0, CURSE_MS);
	return fresh;
}

bool Curses::Any() const {
	return std::any_of(running.begin(), running.end(), [](const Running& r) { return r.leftMs > 0; });
}

unsigned Curses::Advance(int ms) {
	unsigned ended = 0;
	for (size_t c = 0; c < running.size(); c++) {
		Running& r = running[c];
		if (r.leftMs <= 0)
			continue;
		r.leftMs = std::max(0, r.leftMs - ms);
		if (r.leftMs == 0)
			ended |= 1U << c;
	}
	return ended;
}

void Curses::Dump(std::ostream& out) const {
	out << "CURSES " << std::count_if(running.begin(), running.end(), [](const Running& r) { return r.leftMs > 0; });
	for (size_t c = 0; c < running.size(); c++)
		if (running[c].leftMs > 0)
			out << ' ' << c << ' ' << static_cast<int>(running[c].grade) << ' ' << running[c].leftMs;
	out << '\n';
}

void Curses::LoadDump(std::istream& in) {
	EndAll();
	const auto start = in.tellg();
	std::string tag;
	int count = 0;
	if (!(in >> tag) || tag != "CURSES" || !(in >> count)) {
		in.clear();
		in.seekg(start);
		return;
	}
	for (int i = 0; i < count; i++) {
		int c = 0, grade = 0, ms = 0;
		if (!(in >> c >> grade >> ms))
			return;
		if (c < 0 || c >= CURSE_COUNT)
			continue;
		running[static_cast<size_t>(c)] = {
			grade == 1 ? CurseGrade::Greater : CurseGrade::Lesser,
			std::clamp(ms, 0, CURSE_MS)
		};
	}
}

int cursedMight(int might, const Curses& curses) {
	if (!curses.Has(Curse::Weakness))
		return might;
	return cut(might, curses.Greater(Curse::Weakness) ? GREATER_WEAKNESS_MIGHT_CUT : WEAKNESS_MIGHT_CUT);
}

int cursedArmor(int armor, const Curses& curses) {
	return curses.Has(Curse::Vulnerability) ? std::max(0, cut(armor, VULNERABILITY_ARMOR_CUT)) : armor;
}

int cursedMaxHP(int maxHp, const Curses& curses) {
	if (!curses.Has(Curse::Disease))
		return maxHp;
	return std::max(1, cut(maxHp, curses.Greater(Curse::Disease) ? GREATER_DISEASE_MAX_HP_CUT : DISEASE_MAX_HP_CUT));
}

int cursedHit(int damage, const Curses& curses) {
	if (curses.Has(Curse::Clumsiness))
		damage = cut(damage, curses.Greater(Curse::Clumsiness) ? GREATER_CLUMSINESS_DAMAGE_CUT : CLUMSINESS_DAMAGE_CUT);
	return std::max(1, damage);
}
