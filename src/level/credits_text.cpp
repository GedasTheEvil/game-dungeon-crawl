#include "credits_text.h"
#include <fstream>

namespace {
std::string trim(const std::string& s) {
	const size_t first = s.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";
	return s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
} // namespace

std::vector<CreditsGroup> ParseCredits(std::istream& in) {
	std::vector<CreditsGroup> groups;
	std::string raw;
	while (std::getline(in, raw)) {
		const std::string line = trim(raw);
		if (line.empty() || line.starts_with("//"))
			continue;
		if (line.starts_with('#')) {
			groups.push_back({trim(line.substr(1)), {}});
			continue;
		}
		if (groups.empty())
			groups.emplace_back();
		CreditsLine row;
		size_t from = 0;
		while (true) {
			const size_t bar = line.find('|', from);
			row.fields.push_back(trim(line.substr(from, bar == std::string::npos ? bar : bar - from)));
			if (bar == std::string::npos)
				break;
			from = bar + 1;
		}
		groups.back().lines.push_back(std::move(row));
	}
	return groups;
}

std::vector<CreditsGroup> LoadCredits(const std::string& path) {
	std::ifstream file(path);
	return ParseCredits(file);
}
