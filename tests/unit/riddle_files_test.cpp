// The riddle files (plan riddle-duplicate-and-hints): no two riddles share a question.
#include "../../external/doctest/doctest.h"
#include "../../src/core/paths.h"
#include "../../src/level/riddle_file.h"
#include <cctype>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
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
		if (line.starts_with("Q:")) {
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

TEST_CASE("lore-corrections-leftovers: no stargate riddle says swallows the sun or Sobek") {
	std::ifstream in(CONTENT_DIR "riddles/stargate.txt");
	REQUIRE(in.good());
	std::string line;
	while (std::getline(in, line)) {
		CHECK_MESSAGE(!line.contains("swallows the sun"), line);
		CHECK_MESSAGE(!line.contains("Sobek"), line);
	}
}

TEST_CASE("riddle files: every file parses without a problem") {
	for (const auto& entry : std::filesystem::directory_iterator(CONTENT_DIR "riddles")) {
		if (entry.path().extension() != ".txt") {
			continue;
		}
		std::ifstream in(entry.path());
		std::vector<RiddleEntry> riddles;
		std::vector<std::string> errors;
		ParseRiddles(in, entry.path().string(), riddles, errors);
		CHECK_MESSAGE(errors.empty(), entry.path().filename().string() << ": " << (errors.empty() ? "" : errors[0]));
		CHECK(!riddles.empty());
	}
}

TEST_CASE("weighing-of-the-heart: the judgment riddles belong to the Hall of Judgment") {
	std::ifstream in(CONTENT_DIR "riddles/judgment.txt");
	REQUIRE(in.good());
	std::vector<RiddleEntry> riddles;
	std::vector<std::string> errors;
	ParseRiddles(in, "judgment.txt", riddles, errors);
	CHECK(errors.empty());
	// One for each riddle gate of the zone (3 a level on lvl51-54), and a few to spare.
	CHECK(riddles.size() >= 12);
	for (const RiddleEntry& r : riddles) {
		CHECK(r.zone == "hall_of_judgment");
	}
}

TEST_CASE("weighing-of-the-heart: a zone line, an unknown zone") {
	std::istringstream in(
		"Q: common\nA: a1\n\nzone: hall_of_judgment\ntheme: T\nQ: judged\nA: a2\n\nzone: nowhere\nQ: lost\nA: a3\n"
	);
	std::vector<RiddleEntry> riddles;
	std::vector<std::string> errors;
	ParseRiddles(in, "x.txt", riddles, errors);
	REQUIRE(riddles.size() == 3);
	CHECK(riddles[0].zone.empty());
	CHECK(riddles[1].zone == "hall_of_judgment");
	CHECK(riddles[1].theme == "T");
	CHECK(riddles[2].zone.empty()); // an unknown zone: dealt everywhere, and said
	REQUIRE(errors.size() == 1);
	CHECK(errors[0].contains("nowhere"));
}

TEST_CASE("weighing-of-the-heart: a zone level deals its riddles not met yet") {
	std::vector<RiddleEntry> riddles = {
		{"Common", {"c"}, {"c"}, "", "", ""},
		{"Judged", {"j1"}, {"j"}, "", "", "hall_of_judgment"},
		{"Judged", {"j2"}, {"j"}, "", "", "hall_of_judgment"},
		{"Fire", {"f"}, {"f"}, "", "", "lake_of_fire"},
	};
	std::vector<JournalRiddle> met;
	CHECK(zoneRiddlesLeft(riddles, "", met).empty()); // a level outside the zones: the common deck
	CHECK(zoneRiddlesLeft(riddles, "hall_of_judgment", met) == std::vector<size_t>{1, 2});
	CHECK(zoneRiddlesLeft(riddles, "necropolis", met).empty());
	JournalRiddle j1;
	j1.theme = "Judged";
	j1.question = {"j1"};
	met.push_back(j1);
	CHECK(zoneRiddlesLeft(riddles, "hall_of_judgment", met) == std::vector<size_t>{2});
	JournalRiddle j2 = j1;
	j2.question = {"j2"};
	met.push_back(j2);
	CHECK(zoneRiddlesLeft(riddles, "hall_of_judgment", met).empty()); // all met: the common deck again
}
