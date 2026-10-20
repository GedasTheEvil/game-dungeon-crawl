#ifndef JOURNAL_TEXT_H
#define JOURNAL_TEXT_H

#include "journal.h"

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

#endif
