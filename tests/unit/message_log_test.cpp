#include "../../external/doctest/doctest.h"
#include "../../src/ui/message_log.h"
#include <string>
#include <vector>

namespace {
std::vector<std::string> shownAt(const MessageLog& log, int now) {
	std::vector<std::string> lines;
	for (const StatusMessage* m : log.Visible(now))
		lines.push_back(m->Shown());
	return lines;
}
} // namespace

TEST_CASE("status lines stack, newest at the bottom, up to three") {
	MessageLog log;
	log.Push("Found: Sword", StatusTone::Plain, 0, 1);
	log.Push("Dodged", StatusTone::Plain, 100, 1);
	log.Push("Now you are level 2", StatusTone::Good, 200, 1);
	CHECK(shownAt(log, 300) == std::vector<std::string>{"Found: Sword", "Dodged", "Now you are level 2"});
	log.Push("Your feet stick in the web", StatusTone::Plain, 400, 1);
	CHECK(shownAt(log, 500) == std::vector<std::string>{"Dodged", "Now you are level 2", "Your feet stick in the web"});
	CHECK(log.Log().size() == 4);
}

TEST_CASE("each line runs out on its own timer; good news and dangers stay longer") {
	MessageLog log;
	log.Push("You are poisoned!", StatusTone::Danger, 0, 1);
	log.Push("Dodged", StatusTone::Plain, 1000, 1);
	CHECK(shownAt(log, 3999).size() == 2);
	CHECK(shownAt(log, MessageLog::PLAIN_MS + 1000) == std::vector<std::string>{"You are poisoned!"});
	CHECK(shownAt(log, MessageLog::TONED_MS - 1).size() == 1);
	CHECK(shownAt(log, MessageLog::TONED_MS).empty());
	CHECK(log.Log().size() == 2); // the log keeps them
}

TEST_CASE("a level up between two kills does not hide the XP: the gains add up and come to the bottom") {
	MessageLog log;
	log.Push("Gained 30 XP", StatusTone::Plain, 0, 1);
	log.Push("Now you are level 2", StatusTone::Good, 0, 1);
	log.Push("Gained 30 XP", StatusTone::Plain, 1000, 1);
	CHECK(shownAt(log, 1000) == std::vector<std::string>{"Now you are level 2", "Gained 60 XP"});
	CHECK(log.Visible(1000).back()->bornMs == 0); // no second fade-in
	CHECK(shownAt(log, 3500) == std::vector<std::string>{"Now you are level 2", "Gained 60 XP"}); // its time restarted
	log.Push("Riddle answered, got 50 XP", StatusTone::Plain, 1100, 1);
	CHECK(log.Visible(1100).size() == 3); // other words: no merge
}

TEST_CASE("a line told again while it shows counts its repeats; once gone it is a new line") {
	MessageLog log;
	log.Push("Your feet stick in the web", StatusTone::Plain, 0, 1);
	log.Push("Your feet stick in the web", StatusTone::Plain, 500, 1);
	log.Push("Your feet stick in the web", StatusTone::Plain, 900, 1);
	CHECK(shownAt(log, 1000) == std::vector<std::string>{"Your feet stick in the web (x3)"});
	CHECK(log.Log().size() == 1);
	log.Push("Your feet stick in the web", StatusTone::Plain, 900 + MessageLog::PLAIN_MS, 1);
	CHECK(log.Log().size() == 2);
	CHECK(shownAt(log, 900 + MessageLog::PLAIN_MS) == std::vector<std::string>{"Your feet stick in the web"});
}

TEST_CASE("the log keeps the last 50 lines of the level the player is on") {
	MessageLog log;
	for (int i = 0; i < 60; i++)
		log.Push("Line " + std::to_string(i), StatusTone::Plain, i * 10000, 3);
	REQUIRE(log.Log().size() == MessageLog::LOG_SIZE);
	CHECK(log.Log().front().text == "Line 10");
	CHECK(log.Log().back().text == "Line 59");
	log.Push("Gained 5 XP", StatusTone::Plain, 700000, 4);
	CHECK(log.Log().size() == 1);
	CHECK(log.Level() == 4);
}
