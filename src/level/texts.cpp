#include "texts.h"
#include "../core/paths.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>

namespace {
std::string trim(const std::string& s) {
	size_t first = s.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";
	return s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}

bool keyChar(char c) { return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.'; }

bool validKey(const std::string& key) {
	return !key.empty() && key.front() != '.' && key.back() != '.' && std::all_of(key.begin(), key.end(), keyChar);
}
} // namespace

std::string textSlug(std::string_view name) {
	std::string slug;
	for (char c : name) {
		if (c >= 'A' && c <= 'Z')
			c = static_cast<char>(c - 'A' + 'a');
		if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
			slug += c;
		else if (c != '\'' && !slug.empty() && slug.back() != '-')
			slug += '-';
	}
	while (!slug.empty() && slug.back() == '-')
		slug.pop_back();
	return slug;
}

void ParseTexts(
	std::istream& in,
	const std::string& source,
	std::vector<std::pair<std::string, std::string>>& out,
	std::vector<std::string>& errors
) {
	std::string section;
	bool open = false; // the last line was a text: an indented line goes on with it
	int lineNo = 0;
	auto error = [&](const std::string& message) {
		errors.push_back(source + ":" + std::to_string(lineNo) + ": " + message);
	};

	std::string raw;
	while (std::getline(in, raw)) {
		lineNo++;
		const bool indented = !raw.empty() && (raw[0] == ' ' || raw[0] == '\t');
		std::string line = trim(raw);
		if (line.empty()) {
			open = false;
			continue;
		}
		// The fonts only have printable ASCII.
		bool foreign = false;
		for (char& c : line)
			if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126) {
				foreign = true;
				c = '?';
			}
		if (foreign)
			error("non-ASCII character, shown as '?'");
		if (indented) {
			if (open)
				out.back().second += " " + line;
			else
				error("an indented line goes on with the text above it, but there is none: line ignored");
			continue;
		}
		open = false;
		if (line[0] == '#')
			continue;
		if (line.front() == '[') {
			section = line.back() == ']' ? line.substr(1, line.size() - 2) : "";
			if (!validKey(section)) {
				error("expected [section] of a-z, 0-9, '-' and '.', the texts up to the next one ignored");
				section.clear();
			}
			continue;
		}
		const size_t colon = line.find(':');
		const std::string field = colon == std::string::npos ? "" : line.substr(0, colon);
		if (!validKey(field)) {
			error("expected field: text, line ignored");
			continue;
		}
		if (section.empty()) {
			error(field + ": outside a [section], line ignored");
			continue;
		}
		out.emplace_back(section + "." + field, trim(line.substr(colon + 1)));
		open = true;
	}
}

std::vector<std::string> TextBook::Load(const std::string& path) {
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
	if (files.empty())
		errors.push_back(path + ": no *.txt files");
	for (const std::string& file : files) {
		std::ifstream in(file);
		if (!in) {
			errors.push_back(file + ": cannot be read");
			continue;
		}
		std::vector<std::pair<std::string, std::string>> read;
		ParseTexts(in, file, read, errors);
		for (auto& [key, value] : read)
			if (!entries.emplace(key, std::move(value)).second)
				errors.push_back(file + ": " + key + " is there twice, the first one kept");
	}
	return errors;
}

const std::string* TextBook::Find(std::string_view key) const {
	auto at = entries.find(key);
	return at == entries.end() ? nullptr : &at->second;
}

namespace {
struct LoadedTexts {
	TextBook book;
	std::vector<std::string> problems;
	LoadedTexts() { problems = book.Load(CONTENT_DIR "texts"); }
};

const LoadedTexts& loaded() {
	static const LoadedTexts LOADED;
	return LOADED;
}

// The "[key]" stand-ins of the missing keys, kept so their c_str() stays valid.
std::mutex missingLock;
std::map<std::string, std::string, std::less<>>& missing() {
	static std::map<std::string, std::string, std::less<>> keys;
	return keys;
}
} // namespace

const TextBook& gameTexts() { return loaded().book; }

const std::vector<std::string>& textProblems() { return loaded().problems; }

const std::string& gameText(std::string_view key) {
	if (const std::string* found = gameTexts().Find(key))
		return *found;
	const std::scoped_lock lock(missingLock);
	auto at = missing().find(key);
	if (at == missing().end())
		at = missing().emplace(std::string(key), "[" + std::string(key) + "]").first;
	return at->second;
}

std::vector<std::string> missingTexts() {
	const std::scoped_lock lock(missingLock);
	std::vector<std::string> keys;
	for (const auto& entry : missing())
		keys.push_back(entry.first);
	return keys;
}
