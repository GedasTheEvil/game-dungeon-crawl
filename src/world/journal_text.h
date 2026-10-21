#ifndef JOURNAL_TEXT_H
#define JOURNAL_TEXT_H

#include "journal.h"
#include <array>

struct MonsterKind;

// The journal's words (JournalScreen draws them): a new monster's move or a new field note is written here, not in
// the screen.

// One sentence on a move the player saw a creature make, under SAW on its page.
const char* creatureMoveNote(CreatureMove move, const MonsterKind& kind);

struct FieldNoteText {
	const char* title;
	const char* text;
};
// The rules as the game has them (PlayerStats, ItemBag, the mechanisms); keep them in step.
const FieldNoteText& fieldNoteText(FieldNote note);

// A diary page (plan journal-intro): a dated heading, then its paragraphs.
struct DiaryText {
	const char* title;
	std::array<const char*, 3> paragraphs; // null past the last one
};
const DiaryText& diaryText(DiaryPage page);

// A page of the earlier expedition's notebook (plan expedition-notes), in their words: a heading, then a few lines.
struct ExpeditionPageText {
	const char* heading;
	const char* text;
};
// number: isExpeditionPage. The hints are the game's rules (monster_kinds.cpp, the levels): keep them in step.
const ExpeditionPageText& expeditionPageText(int number);

#endif
