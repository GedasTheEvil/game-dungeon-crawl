// The riddle files (plan riddle-duplicate-and-hints): no two riddles share a question.
#include "../../external/doctest/doctest.h"
#include "../../src/core/paths.h"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace {
struct FileRiddle {
	std::string question; // the Q: lines joined, letters and digits only, lower case
};

std::string lettersOnly(const std::string& text) {
	std::string out;
	for (const char c : text) {
		if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
			out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		}
	}
	return out;
}

// Only what the check needs: a Q: line after any other line starts a riddle.
std::vector<FileRiddle> readRiddles(const std::filesystem::path& file) {
	std::vector<FileRiddle> riddles;
	std::ifstream in(file);
	std::string line;
	bool inQuestion = false;
	while (std::getline(in, line)) {
		if (line.rfind("Q:", 0) == 0) {
			if (!inQuestion) {
				riddles.emplace_back();
			}
			riddles.back().question += lettersOnly(line.substr(2));
			inQuestion = true;
			continue;
		}
		inQuestion = false;
	}
	return riddles;
}
} // namespace

TEST_CASE("riddle files: no two riddles share a question") {
	std::map<std::string, std::string> seen; // question -> file
	int count = 0;
	for (const auto& entry : std::filesystem::directory_iterator(CONTENT_DIR "riddles")) {
		if (entry.path().extension() != ".txt") {
			continue;
		}
		const std::string name = entry.path().filename().string();
		for (const FileRiddle& riddle : readRiddles(entry.path())) {
			count++;
			const auto [at, added] = seen.emplace(riddle.question, name);
			CHECK_MESSAGE(added, "duplicate question in " << name << " and " << at->second << ": " << riddle.question);
		}
	}
	CHECK(count > 0);
}
