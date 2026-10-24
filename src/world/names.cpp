#include "names.h"
#include "rng.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <numeric>

namespace {
std::string trim(const std::string& s) {
	size_t first = s.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";
	return s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
} // namespace

std::string NormalizeAnswer(const std::string& text) {
	std::string lower;
	for (char c : text)
		lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	lower = trim(lower);
	for (const char* article : {"a ", "an ", "the "}) {
		std::string prefix(article);
		if (lower.size() > prefix.size() && lower.compare(0, prefix.size(), prefix) == 0) {
			lower = lower.substr(prefix.size());
			break;
		}
	}
	std::string out;
	for (char c : lower)
		if (std::isalnum(static_cast<unsigned char>(c)))
			out += c;
	return out;
}

bool RoyalName::Accepts(const std::string& typed) const {
	return std::find(answers.begin(), answers.end(), NormalizeAnswer(typed)) != answers.end();
}

void ParseNames(
	std::istream& in,
	const std::string& source,
	std::vector<RoyalName>& out,
	std::vector<std::string>& errors
) {
	RoyalName current;
	bool open = false;
	int lineNo = 0;
	auto error = [&](const std::string& message) {
		errors.push_back(source + ":" + std::to_string(lineNo) + ": " + message);
	};
	auto finish = [&]() {
		if (open && current.answers.empty())
			errors.push_back(current.source + ": name " + current.name + " has no A: line, skipped");
		else if (open)
			out.push_back(current);
		current = RoyalName{};
		open = false;
	};

	std::string raw;
	while (std::getline(in, raw)) {
		lineNo++;
		std::string line = trim(raw);
		if (line.empty() || line[0] == '#')
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
		if (key == "name") {
			finish();
			if (value.empty()) {
				error("name: without a name, entry skipped");
				continue;
			}
			current.name = value;
			current.source = source + ":" + std::to_string(lineNo);
			open = true;
		} else if (!open) {
			error(key.empty() ? "expected name:, line ignored" : key + ": before any name: line, ignored");
		} else if (key == "title") {
			current.title = value;
		} else if (key == "A") {
			std::string normalized = NormalizeAnswer(value);
			if (normalized.empty())
				error("answer has no letters or digits, ignored");
			else if (value.size() > MAX_ANSWER_CHARS)
				error("answer longer than " + std::to_string(MAX_ANSWER_CHARS) + " characters, ignored");
			else
				current.answers.push_back(normalized);
		} else if (key == "N") {
			current.note = value;
		} else {
			error("expected name:, title:, A: or N:, line ignored");
		}
	}
	finish();
}

std::vector<std::string> NameList::Load(const std::string& path) {
	std::vector<std::string> files;
	std::error_code ec;
	if (std::filesystem::is_directory(path, ec)) {
		for (const auto& entry : std::filesystem::directory_iterator(path, ec))
			if (entry.path().extension() == ".txt")
				files.push_back(entry.path().string());
		std::sort(files.begin(), files.end()); // directory order is not stable across file systems
	} else {
		files.push_back(path);
	}

	names.clear();
	std::vector<std::string> errors;
	for (const std::string& file : files) {
		std::ifstream in(file);
		if (!in) {
			errors.push_back(file + ": cannot be read");
			continue;
		}
		ParseNames(in, file, names, errors);
	}
	return errors;
}

const RoyalName* NameList::ForLevel(uint32_t seed, int level) const {
	if (names.empty())
		return nullptr;
	std::vector<size_t> order(names.size());
	std::iota(order.begin(), order.end(), 0);
	Rng rng(seed);
	for (size_t i = order.size(); i > 1; i--)
		std::swap(order[i - 1], order[static_cast<size_t>(rng.below(static_cast<int>(i)))]);
	const auto slot = static_cast<size_t>(std::max(level - 1, 0)) % order.size();
	return &names[order[slot]];
}

int nameCartouchePicture(uint32_t seed, int level) {
	Rng rng((static_cast<uint64_t>(seed) << 16) ^ static_cast<uint64_t>(static_cast<uint16_t>(level)));
	return rng.below(NAME_CARTOUCHE_PICTURES);
}
