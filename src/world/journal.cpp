#include "journal.h"
#include <algorithm>
#include <iomanip>

namespace {
constexpr const char* TAG = "journal";
constexpr int VERSION = 1;
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
}

void Journal::Load(std::istream& in) {
	riddles.clear();
	std::string tag;
	int version = 0;
	size_t count = 0;
	if (!(in >> tag) || tag != TAG || !(in >> version >> count) || version != VERSION)
		return;
	for (size_t i = 0; i < count; i++) {
		JournalRiddle r;
		if (!(in >> std::quoted(r.theme)) || !readLines(in, r.question) || !readLines(in, r.answers) ||
			!(in >> std::quoted(r.hint) >> r.level >> r.lateXP >> r.hintShown >> r.solved >> r.solvedLate >>
			  std::quoted(r.written)))
			return; // a broken tail: keep what was read
		riddles.push_back(r);
	}
}
