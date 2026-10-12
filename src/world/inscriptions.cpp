#include "inscriptions.h"
#include <algorithm>
#include <charconv>
#include <filesystem>
#include <fstream>

namespace {
std::string trim(const std::string& s) {
	size_t first = s.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";
	return s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
} // namespace

void ParseInscriptions(std::istream& in, const std::string& source, std::vector<InscriptionText>& out,
					   std::vector<std::string>& errors) {
	InscriptionText current;
	bool open = false;
	int lineNo = 0;
	auto error = [&](const std::string& message) {
		errors.push_back(source + ":" + std::to_string(lineNo) + ": " + message);
	};
	auto finish = [&]() {
		if (open && current.text.empty())
			errors.push_back(current.source + ": inscription " + std::to_string(current.id) +
							 " has no T: line, skipped");
		else if (open)
			out.push_back(current);
		current = InscriptionText{};
		open = false;
	};

	std::string raw;
	while (std::getline(in, raw)) {
		lineNo++;
		std::string line = trim(raw);
		if (line.empty() || line[0] == '#')
			continue;
		size_t colon = line.find(':');
		std::string key = colon == std::string::npos ? "" : line.substr(0, colon);
		std::string value = colon == std::string::npos ? "" : trim(line.substr(colon + 1));
		if (key == "G" || key == "style") // the hieroglyphs and their frame: the atlas tool's
			continue;
		// The fonts only have printable ASCII.
		for (char& c : value)
			if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126) {
				error("non-ASCII character, shown as '?'");
				c = '?';
			}
		if (key == "id") {
			finish();
			int id = 0;
			auto [end, ec] = std::from_chars(value.data(), value.data() + value.size(), id);
			if (ec != std::errc{} || end != value.data() + value.size() || !isInscriptionId(id)) {
				error("id must be 1 to " + std::to_string(INSCRIPTION_MAX) + ", entry skipped");
				continue;
			}
			current.id = id;
			current.source = source + ":" + std::to_string(lineNo);
			open = true;
		} else if (!open) {
			error(key.empty() ? "expected id:, line ignored" : key + ": before any id: line, ignored");
		} else if (key == "title") {
			current.title = value;
		} else if (key == "T") {
			current.text.push_back(value);
		} else if (key == "N") {
			current.note = value;
		} else {
			error("expected id:, title:, style:, G:, T: or N:, line ignored");
		}
	}
	finish();
}

std::vector<std::string> InscriptionBook::Load(const std::string& path) {
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

	entries.clear();
	std::vector<std::string> errors;
	std::vector<InscriptionText> read;
	for (const std::string& file : files) {
		std::ifstream in(file);
		if (!in) {
			errors.push_back(file + ": cannot be read");
			continue;
		}
		ParseInscriptions(in, file, read, errors);
	}
	for (const InscriptionText& i : read) {
		if (const InscriptionText* first = Find(i.id)) {
			errors.push_back(i.source + ": id " + std::to_string(i.id) + " is taken by " + first->source + ", skipped");
			continue;
		}
		entries.push_back(i);
	}
	return errors;
}

const InscriptionText* InscriptionBook::Find(int id) const {
	for (const InscriptionText& i : entries)
		if (i.id == id)
			return &i;
	return nullptr;
}
