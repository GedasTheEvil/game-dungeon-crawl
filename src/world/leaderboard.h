#ifndef LEADERBOARD_H
#define LEADERBOARD_H

// The leaderboard (plan leaderboard): one record per won campaign run, in saves/leaderboard.txt, never part of a save
// game. The file is append only, one block per record ("[run]", then key=value lines), so old records stay readable
// when new statistics arrive. GL-free: the Hall of Records screen (ui/records_view.cpp) draws it.

#include <array>
#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

class PlayerStats;

// One won run: who, when, which build, and every statistic of RunStats and the player at the win.
struct RunRecord {
	std::string name;
	std::string date; // "2026-10-13"
	std::string version; // the build's git hash
	int playMs = 0;
	int xp = 0;
	int playerLevel = 1;
	int maxHp = 0;
	int stamina = 0;
	int armor = 0;
	int might = 0;
	std::vector<std::string> skills; // names, as on the Heka screen
	std::vector<std::pair<std::string, int>> slain; // monster name, count
	std::vector<std::pair<std::string, int>> potions; // potion name, count
	std::vector<int> levelMs; // [level - 1]: time in play there
	int inscriptions = 0;
	int treasures = 0;
	int riddles = 0;
	int damageDealt = 0;
	int damageTaken = 0;
	int biggestHit = 0;
	int poisoned = 0;
	int cursed = 0;
	bool placeholder = false; // the empty board's "Spirit of Exploration", never written

	[[nodiscard]] int SlainTotal() const;
	[[nodiscard]] int PotionsTotal() const;
};

// The run of `stats` at its win.
[[nodiscard]] RunRecord recordOf(const PlayerStats& stats, std::string name, std::string date, std::string version);

// The orders of the board, one sort button each.
enum class RecordOrder : std::uint8_t { Time, Xp, Explored };
constexpr int RECORD_ORDER_COUNT = 3;
constexpr std::array<const char*, RECORD_ORDER_COUNT> RECORD_ORDER_NAMES = {"Time", "XP", "Explored"};

// Exploration score weights (user, 2026-10-13): riddles are rarer and harder.
constexpr int EXPLORE_INSCRIPTION_POINTS = 10;
constexpr int EXPLORE_TREASURE_POINTS = 10;
constexpr int EXPLORE_RIDDLE_POINTS = 30;
[[nodiscard]] int explorationScore(const RunRecord& r);

[[nodiscard]] std::string formatPlayTime(int ms); // "h:mm:ss"
// The score column in that order: the play time, the XP, the exploration score; "-" for the placeholder.
[[nodiscard]] std::string scoreText(const RunRecord& r, RecordOrder order);
// Indexes into `records`, best first: time ascending; XP and exploration descending, ties the faster run first.
[[nodiscard]] std::vector<int> rankRecords(const std::vector<RunRecord>& records, RecordOrder order);

// The empty board's row: shown while no run has won, never written.
[[nodiscard]] RunRecord placeholderRecord();

// Reads every record; a broken line or a record without a name is skipped with a message in `warnings`.
[[nodiscard]] std::vector<RunRecord> readRecords(std::istream& in, std::vector<std::string>& warnings);
void writeRecord(std::ostream& out, const RunRecord& r);

// The leaderboard file.
class Leaderboard {
  public:
	explicit Leaderboard(std::string path = "saves/leaderboard.txt") : path(std::move(path)) {}
	void SetPath(std::string to) { path = std::move(to); } // scenarios: their output directory, never saves/
	void Load(); // a missing file is an empty board
	bool Add(const RunRecord& r); // appends it to the file and the list; false if the file cannot be written
	[[nodiscard]] const std::vector<RunRecord>& Records() const { return records; }
	// The records, or the placeholder alone on an empty board.
	[[nodiscard]] std::vector<RunRecord> Shown() const;
	[[nodiscard]] std::string LastName() const; // the latest record's name, the default of the next one

  private:
	std::string path;
	std::vector<RunRecord> records;
};

#endif
