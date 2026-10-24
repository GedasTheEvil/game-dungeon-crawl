#include "journal.h"
#include <algorithm>
#include <iomanip>

namespace {
constexpr const char* TAG = "journal";
// 1: riddles only, 2: and creatures, 3: and field notes, 4: and the damage types tried, 5: notes per potion group and
// damage type (older field notes are dropped: their numbers meant other notes), 6: and the inscriptions read, 7: and
// whether each was translated, 8: fire among the damage types (the poison's tried bit moved up one), 9: and each
// inscription's boon, 10: and its name cartouche picture, 11: and the diary pages, 12: and the
// expedition's pages
constexpr int VERSION = 13;
constexpr int NOTES_PER_KIND_VERSION = 5;
constexpr int INSCRIPTIONS_VERSION = 6;
constexpr int TRANSLATED_VERSION = 7;
constexpr int FIRE_VERSION = 8;
constexpr int BOON_VERSION = 9;
constexpr int PICTURE_VERSION = 10;
constexpr int DIARY_VERSION = 11;
constexpr int PAGES_VERSION = 12;
constexpr int GLYPHS_VERSION = 13; // a name cartouche's signs instead of its picture
constexpr unsigned OLD_TRIED_POISON = 1U << 3; // before fire
constexpr int MIN_LATE_XP = 50;

void writeLines(std::ostream& out, const std::vector<std::string>& lines) {
	out << lines.size();
	for (const std::string& l : lines)
		out << ' ' << std::quoted(l);
}

bool readLines(std::istream& in, std::vector<std::string>& lines) {
	size_t count = 0;
	if (!(in >> count))
		return false;
	lines.resize(count);
	for (std::string& l : lines)
		if (!(in >> std::quoted(l)))
			return false;
	return true;
}
} // namespace

int lateRiddleXP(int gateXP) { return std::max(gateXP / 10, MIN_LATE_XP); }

size_t Journal::MeetRiddle(const JournalRiddle& riddle) {
	for (size_t i = 0; i < riddles.size(); i++)
		if (riddles[i].SameRiddle(riddle))
			return i;
	riddles.push_back(riddle);
	return riddles.size() - 1;
}

void Journal::Solve(size_t index, bool late, const std::string& typed) {
	JournalRiddle& r = riddles.at(index);
	if (r.solved)
		return; // the first answer stays written down
	r.solved = true;
	r.solvedLate = late;
	r.written = typed;
}

JournalCreature& Journal::creature(int type, int level) {
	for (JournalCreature& c : creatures)
		if (c.type == type)
			return c;
	JournalCreature c;
	c.type = type;
	c.level = level;
	creatures.push_back(c);
	return creatures.back();
}

bool Journal::TryDamage(int type, int level, DamageType damage) {
	JournalCreature& c = creature(type, level);
	if (c.Tried(damage))
		return false;
	c.tried |= 1U << static_cast<unsigned>(damage);
	return true;
}

bool Journal::TryPoison(int type, int level) {
	JournalCreature& c = creature(type, level);
	if (c.TriedPoison())
		return false;
	c.tried |= JournalCreature::TRIED_POISON;
	return true;
}

std::optional<FieldNote> skillNote(SkillId skill) {
	switch (skill) {
	case SkillId::SerketsFavour:
		return FieldNote::SerketsFavour;
	case SkillId::SharpEye:
		return FieldNote::SharpEye;
	case SkillId::FavourOfHapi:
		return FieldNote::FavourOfHapi;
	case SkillId::BlessingOfIsis:
		return FieldNote::BlessingOfIsis;
	default:
		return std::nullopt;
	}
}

FieldNote damageNote(DamageType type) {
	switch (type) {
	case DamageType::Blunt:
		return FieldNote::Blunt;
	case DamageType::Slash:
		return FieldNote::Slash;
	case DamageType::Pierce:
		return FieldNote::Pierce;
	case DamageType::Fire:
		break;
	}
	return FieldNote::Fire;
}

FieldNote potionNote(ItemKind potion) { return potionDef(potion).note; }

void Journal::LearnNote(FieldNote note) {
	if (std::find(notes.begin(), notes.end(), note) == notes.end())
		notes.push_back(note);
}

bool Journal::ReadInscription(const JournalInscription& inscription) {
	for (JournalInscription& i : inscriptions)
		if (i.SameInscription(inscription)) {
			if (i.translated || !inscription.translated)
				return false;
			i = inscription;
			return true;
		}
	inscriptions.push_back(inscription);
	return true;
}

const JournalInscription* Journal::FindInscription(const JournalInscription& inscription) const {
	for (const JournalInscription& i : inscriptions)
		if (i.SameInscription(inscription))
			return &i;
	return nullptr;
}

std::vector<Boon> Journal::TranslateInscriptions() {
	std::vector<Boon> boons;
	for (JournalInscription& i : inscriptions)
		if (!i.translated) {
			i.translated = true;
			boons.push_back(i.boon);
		}
	return boons;
}

void Journal::WriteDiary(DiaryPage page) {
	if (std::find(diary.begin(), diary.end(), page) == diary.end())
		diary.push_back(page);
}

bool Journal::FindPage(int number, int level) {
	if (!isExpeditionPage(number))
		return false;
	auto at = std::find_if(pages.begin(), pages.end(), [number](const ExpeditionPage& p) {
		return p.number >= number;
	});
	if (at != pages.end() && at->number == number)
		return false;
	pages.insert(at, {number, level});
	return true;
}

void Journal::Begin() {
	Clear();
	WriteDiary(DiaryPage::Arrival);
	WriteDiary(DiaryPage::Descent);
	introPending = true;
}

void Journal::Dump(std::ostream& out) const {
	out << TAG << ' ' << VERSION << ' ' << riddles.size() << '\n';
	for (const JournalRiddle& r : riddles) {
		out << std::quoted(r.theme) << ' ';
		writeLines(out, r.question);
		out << ' ';
		writeLines(out, r.answers);
		out << ' ' << std::quoted(r.hint) << ' ' << r.level << ' ' << r.lateXP << ' ' << r.hintShown << ' ' << r.solved
			<< ' ' << r.solvedLate << ' ' << std::quoted(r.written) << '\n';
	}
	out << creatures.size() << '\n';
	for (const JournalCreature& c : creatures)
		out << c.type << ' ' << c.level << ' ' << c.killed << ' ' << c.hitBy << ' ' << c.moves << ' ' << c.tried
			<< '\n';
	out << notes.size();
	for (FieldNote n : notes)
		out << ' ' << static_cast<int>(n);
	out << '\n';
	out << inscriptions.size() << '\n';
	for (const JournalInscription& i : inscriptions) {
		out << i.id << ' ' << i.level << ' ' << std::quoted(i.title) << ' ';
		writeLines(out, i.text);
		out << ' ' << std::quoted(i.note) << ' ' << i.translated << ' ' << static_cast<int>(i.boon.kind) << ' '
			<< i.boon.amount << ' ';
		writeLines(out, i.glyphs);
		out << '\n';
	}
	out << diary.size();
	for (DiaryPage d : diary)
		out << ' ' << static_cast<int>(d);
	out << '\n';
	out << pages.size();
	for (const ExpeditionPage& p : pages)
		out << ' ' << p.number << ' ' << p.level;
	out << '\n';
}

void Journal::Load(std::istream& in) {
	Clear();
	std::string tag;
	int version = 0;
	size_t count = 0;
	if (!(in >> tag) || tag != TAG || !(in >> version >> count) || version < 1 || version > VERSION)
		return;
	for (size_t i = 0; i < count; i++) {
		JournalRiddle r;
		if (!(in >> std::quoted(r.theme)) || !readLines(in, r.question) || !readLines(in, r.answers) ||
			!(in >> std::quoted(r.hint) >> r.level >> r.lateXP >> r.hintShown >> r.solved >> r.solvedLate >>
			  std::quoted(r.written)))
			return; // a broken tail: keep what was read
		riddles.push_back(r);
	}
	if (version < 2 || !(in >> count))
		return;
	for (size_t i = 0; i < count; i++) {
		JournalCreature c;
		if (!(in >> c.type >> c.level >> c.killed >> c.hitBy >> c.moves) || (version >= 4 && !(in >> c.tried)))
			return;
		if (version < FIRE_VERSION && (c.tried & OLD_TRIED_POISON) != 0)
			c.tried = (c.tried & ~OLD_TRIED_POISON) | JournalCreature::TRIED_POISON;
		creatures.push_back(c);
	}
	if (version < 3 || !(in >> count))
		return;
	if (version < NOTES_PER_KIND_VERSION)
		return;
	for (size_t i = 0; i < count; i++) {
		int note = 0;
		if (!(in >> note))
			return;
		if (note >= 0 && note < FIELD_NOTE_COUNT)
			LearnNote(static_cast<FieldNote>(note));
	}
	if (version < INSCRIPTIONS_VERSION || !(in >> count))
		return;
	for (size_t i = 0; i < count; i++) {
		JournalInscription e;
		if (!(in >> e.id >> e.level >> std::quoted(e.title)) || !readLines(in, e.text) ||
			!(in >> std::quoted(e.note)) || (version >= TRANSLATED_VERSION && !(in >> e.translated)))
			return;
		int kind = 0;
		if (version >= BOON_VERSION && !(in >> kind >> e.boon.amount))
			return;
		int picture = 0; // an old name cartouche's picture: its signs did not spell the name, dropped
		if (version >= GLYPHS_VERSION ? !readLines(in, e.glyphs) : version >= PICTURE_VERSION && !(in >> picture))
			return;
		if (kind > 0 && kind < BOON_KIND_COUNT && e.boon.amount > 0)
			e.boon.kind = static_cast<BoonKind>(kind);
		else
			e.boon = Boon{};
		inscriptions.push_back(e);
	}
	if (version < DIARY_VERSION || !(in >> count))
		return;
	for (size_t i = 0; i < count; i++) {
		int page = 0;
		if (!(in >> page))
			return;
		if (page >= 0 && page < DIARY_PAGE_COUNT)
			WriteDiary(static_cast<DiaryPage>(page));
	}
	if (version < PAGES_VERSION || !(in >> count))
		return;
	for (size_t i = 0; i < count; i++) {
		ExpeditionPage p;
		if (!(in >> p.number >> p.level))
			return;
		FindPage(p.number, p.level);
	}
}
