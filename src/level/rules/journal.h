#ifndef JOURNAL_H
#define JOURNAL_H

#include "journal_ids.h"
#include "curses.h"
#include "damage.h"
#include "../inscriptions.h"
#include "items.h"
#include "../level.h"
#include "run_stats.h"
#include "skills.h"
#include "../shrines.h"
#include <istream>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

// The archaeologist's notebook (plan journal-sections): what the player went through, kept per save game: creatures,
// diary, riddles, field notes, inscriptions and the pages of the earlier expedition.

// A riddle met at a gate. A copy, not an index into content/riddles/*.txt: those files can change between saves.
struct JournalRiddle {
	std::string theme;
	std::vector<std::string> question; // one entry per Q: line
	std::vector<std::string> answers; // normalized (NormalizeAnswer)
	std::string hint;
	int level = 0; // campaign level of the gate
	int lateXP = 0; // a late answer from the journal; fixed when the gate is met
	bool hintShown = false; // the hint is written down only once it was shown
	bool solved = false;
	bool solvedLate = false; // answered from the journal, not at the gate
	std::string written; // the answer as the player typed it

	[[nodiscard]] bool SameRiddle(const JournalRiddle& other) const {
		return theme == other.theme && question == other.question;
	}
};

// A wall inscription read (plan hieroglyph-cartouches). A copy of its text, like a riddle: content/inscriptions/*.txt
// can change between saves.
struct JournalInscription {
	int id = 0; // the text id (inscriptions.h); its signs on the page come from the text of this id
	std::string title;
	std::vector<std::string> text;
	std::string note;
	int level = 0; // campaign level where it was read first
	// False for a copy made without the hieroglyph skill: the glyphs only, the text stays hidden until the skill is
	// learnt (plan skill-hieroglyphs).
	bool translated = true;
	Boon boon; // given when it is translated (plan inscription-boons)
	// A name cartouche (id NAME_CARTOUCHE_ID, plan secret-name-gates): the name in signs, a copy of its G: line (plan
	// composed-inscriptions); one per level, so the level tells them apart. Empty for any other inscription.
	std::vector<std::string> glyphs;
	InscriptionStyle frame = InscriptionStyle::Cartouche; // a name's frame (plan royal-name-signs): cartouche, serekh

	[[nodiscard]] bool SameInscription(const JournalInscription& other) const {
		return id == other.id && (id != NAME_CARTOUCHE_ID || level == other.level);
	}
};

// What the archaeologist knows about one monster type (MonsterTypeId). Every note comes from a meeting, never from a
// kill count (plan monster-journal).
struct JournalCreature {
	int type = 0;
	int level = 0; // campaign level of the first meeting
	bool killed = false; // name, description and HP
	bool hitBy = false; // how hard it hits
	unsigned moves = 0; // CreatureMove bits
	// DamageType bits: hit with a weapon of that main type, its resistance is known. TRIED_POISON: the venom amulet's
	// poison tried on it, its poison resistance is known.
	unsigned tried = 0;
	static constexpr unsigned TRIED_POISON = 1U << DAMAGE_TYPE_COUNT;
	[[nodiscard]] bool Saw(CreatureMove m) const { return (moves & (1U << static_cast<unsigned>(m))) != 0; }
	[[nodiscard]] bool Tried(DamageType t) const { return (tried & (1U << static_cast<unsigned>(t))) != 0; }
	[[nodiscard]] bool TriedPoison() const { return (tried & TRIED_POISON) != 0; }
};

// The archaeologist's own entries (plan journal-intro), the Diary section: one page each, in the order written.
enum class DiaryPage : unsigned char {
	Arrival, // the prologue: the statue, the ankh, the torch on the wall
	Descent, // the prologue's second page: the club, the way down, the ankh at the bottom
	Dawn, // the epilogue (plan win-ending-flow): up into the dawn with the ankh, written at the win
	// The epilogue answers D. J. (plan expedition-ending), written at the win after Dawn: the last page was found
	// (Answer), the back cover too (AnswerBackCover).
	Answer,
	AnswerBackCover,
	// The epilogue answers the scales (plan weighing-of-the-heart), written at the win after Dawn when the heart was
	// weighed: the verdict's page (judgment.h, verdictPage).
	ScalesLight,
	ScalesHeavy,
	// The other epilogue (plan ending-choice): the gods renounced as demons, the ankh broken, up into an ordinary
	// sunrise. Written at the win instead of Dawn.
	DawnRenounced,
};
constexpr int DIARY_PAGE_COUNT = 8;

// The earlier expedition's notebook (plan expedition-notes): torn pages found over the campaign, numbered in order of
// depth (Page tiles, level.h). Its own section of the journal, someone else's hand.
struct ExpeditionPage {
	int number = 0; // isExpeditionPage
	int level = 0; // campaign level where it was found
};

// The journal's sections with entries, in the book's order (JournalScreen::Section starts with them).
enum class JournalPart : unsigned char { Diary, Creatures, Riddles, FieldNotes, Inscriptions, Expedition };
constexpr int JOURNAL_PART_COUNT = 6;

// One tenth of the gate's reward (riddleXP), at least 50.
[[nodiscard]] int lateRiddleXP(int gateXP);

class Journal {
  public:
	// A riddle asked at a gate. A riddle met again keeps its entry (the late XP and the level of the first time).
	// Returns its index in Riddles().
	size_t MeetRiddle(const JournalRiddle& riddle);
	void ShowHint(size_t index) { riddles.at(index).hintShown = true; }
	void Solve(size_t index, bool late, const std::string& typed);
	[[nodiscard]] const std::vector<JournalRiddle>& Riddles() const { return riddles; }

	// A monster seen on screen; each of the others sees it too. Creatures keep the order they were first seen in.
	void SeeCreature(int type, int level) { creature(type, level); }
	// True the first time: its name, photo and HP are written down.
	bool KillCreature(int type, int level);
	void HitByCreature(int type, int level);
	// Kept for the tests and the save; the page no longer writes moves down, so not a new note.
	void SeeMove(int type, int level, CreatureMove move);
	// Hit with a weapon of this main type. True the first time: a new note.
	bool TryDamage(int type, int level, DamageType damage);
	// Poisoned by the player (it took or not). True the first time: a new note.
	bool TryPoison(int type, int level);
	[[nodiscard]] const std::vector<JournalCreature>& Creatures() const { return creatures; }

	// Written once; the notes keep the order they were learnt in.
	void LearnNote(FieldNote note);
	[[nodiscard]] const std::vector<FieldNote>& Notes() const { return notes; }

	// Written once, the first time it is read or copied; the entries keep the order they were read in. True the first
	// time, or when a copy gets translated by this call (inscription.translated).
	bool ReadInscription(const JournalInscription& inscription);
	// The entry of this inscription (SameInscription), null before it was read or copied.
	[[nodiscard]] const JournalInscription* FindInscription(const JournalInscription& inscription) const;
	// The hieroglyph skill learnt: every copy shows its text. Returns the boon of each copy translated (none for most),
	// one entry per copy.
	std::vector<Boon> TranslateInscriptions();
	[[nodiscard]] const std::vector<JournalInscription>& Inscriptions() const { return inscriptions; }

	// Written once; the pages keep the order they were written in.
	void WriteDiary(DiaryPage page);
	// At the win: Dawn (DawnRenounced for that ending, plan ending-choice), then the scales' page if the heart was
	// weighed, then the answer to D. J. if his last page was found.
	void WriteEpilogue(RunEnding ending);
	[[nodiscard]] bool HasNote(FieldNote note) const;
	[[nodiscard]] bool HasPage(int number) const;
	[[nodiscard]] const std::vector<DiaryPage>& Diary() const { return diary; }

	// A torn page picked up; the pages keep their numbers' order. True the first time.
	bool FindPage(int number, int level);
	[[nodiscard]] const std::vector<ExpeditionPage>& Pages() const { return pages; }

	// New entries (plan journal-new-mark): an entry written or filled in since its page was last shown is new, until
	// the screen marks it seen. Saved. Index: in the part's list (Creatures(), Notes(), ...).
	[[nodiscard]] bool IsNew(JournalPart part, size_t index) const;
	[[nodiscard]] bool AnyNew(JournalPart part) const;
	[[nodiscard]] bool AnyNew() const { return !fresh.empty(); }
	void MarkSeen(JournalPart part, size_t index);

	// A new game: an empty journal with the prologue written, and the screen asked to open it (plan journal-intro).
	void Begin();
	// True once after Begin: the game opens the journal on the Diary. A loaded journal never asks.
	bool TakeIntro() { return std::exchange(introPending, false); }

	void Clear() {
		riddles.clear();
		creatures.clear();
		notes.clear();
		inscriptions.clear();
		diary.clear();
		pages.clear();
		fresh.clear();
		introPending = false;
	}

	// Saved after the dungeon. Saves from before the journal end before the tag: the journal stays empty.
	void Dump(std::ostream& out) const;
	void Load(std::istream& in);

  private:
	std::vector<JournalRiddle> riddles;
	std::vector<JournalCreature> creatures;
	std::vector<FieldNote> notes;
	std::vector<JournalInscription> inscriptions;
	std::vector<DiaryPage> diary;
	std::vector<ExpeditionPage> pages;
	// The new entries: a part and its entry's key, one that stays when entries are added (keyOf).
	std::set<std::pair<JournalPart, int>> fresh;
	bool introPending = false; // not saved

	JournalCreature& creature(int type, int level);
	// The entry's key: a creature's type, a note's or diary page's number, an expedition page's number; the index for
	// riddles and inscriptions (their lists only grow at the end).
	[[nodiscard]] int keyOf(JournalPart part, size_t index) const;
	void markNew(JournalPart part, int key) { fresh.emplace(part, key); }
	// The entries of Load, without the new marks; returns the save's version, 0 for none.
	int loadEntries(std::istream& in);
};

#endif
