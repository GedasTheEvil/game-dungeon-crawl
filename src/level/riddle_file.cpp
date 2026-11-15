// The riddle files (docs/riddles.md): parsing, and which riddles a zone level deals (plan weighing-of-the-heart).
#include "riddle_file.h"
#include "campaign.h"
#include "names.h"
#include <algorithm>
#include <filesystem>

namespace {

std::string trim(const std::string& s) {
	size_t first = s.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";
	return s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}

} // namespace

void ParseRiddles(
	std::istream& in,
	const std::string& source,
	std::vector<RiddleEntry>& out,
	std::vector<std::string>& errors
) {
	std::string theme = std::filesystem::path(source).stem().string();
	std::string zone;
	RiddleEntry current;
	int lineNo = 0;
	auto error = [&](const std::string& message) {
		errors.push_back(source + ":" + std::to_string(lineNo) + ": " + message);
	};
	auto finish = [&]() {
		if (!current.question.empty() && current.answers.empty())
			errors.push_back(current.source + ": riddle has no A: line, skipped");
		else if (!current.question.empty())
			out.push_back(current);
		current = RiddleEntry{};
	};

	std::string raw;
	while (std::getline(in, raw)) {
		lineNo++;
		std::string line = trim(raw);
		if (line.empty()) {
			finish();
			continue;
		}
		if (line[0] == '#')
			continue;
		// The fonts only have printable ASCII.
		for (char& c : line)
			if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126) {
				error("non-ASCII character, shown as '?'");
				c = '?';
			}

		size_t colon = line.find(':');
		std::string key = colon == std::string::npos ? "" : line.substr(0, colon);
		std::string value = colon == std::string::npos ? "" : trim(line.substr(colon + 1));
		if (key == "theme") {
			finish();
			theme = value;
		} else if (key == "zone") {
			finish();
			zone = value;
			if (!isZone(zoneBySlug(zone))) {
				error("no zone named '" + zone + "', its riddles dealt everywhere");
				zone.clear();
			}
		} else if (key == "Q") {
			if (!current.answers.empty())
				finish();
			if (current.question.empty()) {
				current.theme = theme;
				current.zone = zone;
				current.source = source + ":" + std::to_string(lineNo);
			}
			current.question.push_back(value);
		} else if (key == "A" || key == "H") {
			if (current.question.empty()) {
				error(key + ": before any Q: line, ignored");
				continue;
			}
			if (key == "H") {
				current.hint = value;
				continue;
			}
			std::string normalized = NormalizeAnswer(value);
			if (normalized.empty())
				error("answer has no letters or digits, ignored");
			else if (value.size() > MAX_ANSWER_CHARS)
				error("answer longer than " + std::to_string(MAX_ANSWER_CHARS) + " characters, ignored");
			else
				current.answers.push_back(normalized);
		} else {
			error("expected theme:, zone:, Q:, A: or H:, line ignored");
		}
	}
	finish();
}

std::vector<size_t> zoneRiddlesLeft(
	const std::vector<RiddleEntry>& riddles,
	const std::string& zone,
	const std::vector<JournalRiddle>& met
) {
	std::vector<size_t> left;
	if (zone.empty())
		return left;
	for (size_t i = 0; i < riddles.size(); i++) {
		const RiddleEntry& r = riddles[i];
		const auto seen = [&](const JournalRiddle& j) { return j.theme == r.theme && j.question == r.question; };
		if (r.zone == zone && std::ranges::none_of(met, seen))
			left.push_back(i);
	}
	return left;
}
