#ifndef JUDGMENT_H
#define JUDGMENT_H

#include "journal.h"
#include "level.h"
#include "run_stats.h"

// The weighing of the heart (plan weighing-of-the-heart): the scales on the last level weigh the run. Never a game
// over: the verdict is a field note, and the epilogue's page after Dawn answers it (Journal::WriteEpilogue).
enum class Verdict : unsigned char { Light, Heavy };

// What makes the heart light: reading the walls, answering the assessors, finding the hidden names.
[[nodiscard]] inline int heartLight(const RunStats& run) {
	return run.inscriptions + 2 * run.riddles + 3 * run.secrets;
}
// What makes it heavy: the deaths, and the servant figures smashed (a coffin opened is excavation, not robbery).
[[nodiscard]] inline int heartHeavy(const RunStats& run) { return 5 * run.deaths + 2 * run.Slain(MonsterUshabti); }
[[nodiscard]] inline Verdict weighHeart(const RunStats& run) {
	return heartLight(run) >= heartHeavy(run) ? Verdict::Light : Verdict::Heavy;
}
[[nodiscard]] constexpr FieldNote verdictNote(Verdict v) {
	return v == Verdict::Light ? FieldNote::HeartLight : FieldNote::HeartHeavy;
}
[[nodiscard]] constexpr DiaryPage verdictPage(Verdict v) {
	return v == Verdict::Light ? DiaryPage::ScalesLight : DiaryPage::ScalesHeavy;
}

#endif
