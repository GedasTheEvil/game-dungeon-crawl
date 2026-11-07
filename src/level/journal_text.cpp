#include "journal_text.h"
#include "items.h"
#include "monster_kinds.h"
#include "texts.h"
#include <algorithm>
#include <array>

const char* monsterNote(const MonsterKind& kind) {
	return gameText("monster." + textSlug(kind.label) + ".note").c_str();
}

namespace {
// The [note.<slug>] of each FieldNote but the amulets' (note.<amulet model>), in FieldNote order.
constexpr std::array<const char*, static_cast<size_t>(FieldNote::AmuletFirst)> FIELD_NOTE_SLUGS = {
	"health",
	"levels",
	"stamina",
	"keys",
	"poison",
	"weapons",
	"blunt",
	"slash",
	"pierce",
	"health-potions",
	"stamina-potions",
	"might",
	"armor",
	"life",
	"antidote",
	"dart-traps",
	"resistance",
	"heka",
	"serkets-favour",
	"sharp-eye",
	"curse-weakness",
	"curse-clumsiness",
	"curse-vulnerability",
	"curse-fatigue",
	"curse-disease",
	"curse-slowness",
	"curse-palsy",
	"curse-insanity",
	"fire",
	"favour-of-hapi",
	"blessing-of-isis",
	"water-blessing",
	"canopic-jars",
	"jumped-bite",
	"sand",
	"quicksand",
	"sun-mirrors",
	"trapped-chests",
	"tomb-curses",
	"hour-1",
	"hour-2",
	"hour-3",
	"hour-4",
	"hour-5",
	"hour-6",
	"hour-7",
	"hour-8",
	"hour-9",
	"hour-10",
	"hour-11",
	"hour-12",
};

// The [note.<slug>] of each FieldNote after the amulets' up to the rings' (note.ring-<gem slug>), in FieldNote order.
constexpr std::array<const char*, static_cast<int>(FieldNote::RingFirst) - static_cast<int>(FieldNote::AmuletLast) - 1>
	LATE_NOTE_SLUGS = {
		"stun",
		"rings",
		"stargate",
		"dial-device",
		"coffins",
		"sandstorm",
};
// ... and after the rings', in FieldNote order.
constexpr std::array<const char*, FIELD_NOTE_COUNT - static_cast<int>(FieldNote::RingLast) - 1> AFTER_RING_SLUGS = {
	"shrine-of-isis",  "wall-spikes",	  "fire-jets",		 "poison-drips",	 "dodge",
	"power-hit",	   "flaming-arrows",  "words-of-thoth",	 "ka-of-osiris",	 "strength-of-montu",
	"djed-of-ptah",	   "breath-of-shu",	  "speed-of-mafdet", "wisdom-of-seshat", "disarm-trap",
	"quiver-of-neith", "shrine-of-montu", "shrine-of-ptah",	 "shrine-of-amun",	 "shrine-of-serket",
};

std::string fieldNoteKey(int note) {
	if (const int after = note - static_cast<int>(FieldNote::RingLast) - 1; after >= 0)
		return std::string("note.") + AFTER_RING_SLUGS[static_cast<size_t>(after)];
	if (const int ring = note - static_cast<int>(FieldNote::RingFirst); ring >= 0)
		return std::string("note.ring-") + ringGemSlug(static_cast<RingGem>(ring));
	const int amulet = note - static_cast<int>(FieldNote::AmuletFirst);
	const int late = note - static_cast<int>(FieldNote::AmuletLast) - 1;
	if (late >= 0)
		return std::string("note.") + LATE_NOTE_SLUGS[static_cast<size_t>(late)];
	return "note." + (amulet >= 0 ? textSlug(amuletModel(static_cast<AmuletType>(amulet)))
								  : std::string(FIELD_NOTE_SLUGS[static_cast<size_t>(note)]));
}
} // namespace

const FieldNoteText& fieldNoteText(FieldNote note) {
	static const std::array<FieldNoteText, FIELD_NOTE_COUNT> NOTES = [] {
		std::array<FieldNoteText, FIELD_NOTE_COUNT> notes{};
		for (int n = 0; n < FIELD_NOTE_COUNT; n++) {
			const std::string key = fieldNoteKey(n);
			notes[static_cast<size_t>(n)] = {gameText(key + ".title").c_str(), gameText(key + ".text").c_str()};
		}
		return notes;
	}();
	return NOTES[static_cast<size_t>(note)];
}

const DiaryText& diaryText(DiaryPage page) {
	// [diary.<slug>], by DiaryPage: a title, then p1, p2, p3 (the ones there are).
	static constexpr std::array<const char*, DIARY_PAGE_COUNT> SLUGS = {"arrival", "descent", "dawn"};
	static const std::array<DiaryText, DIARY_PAGE_COUNT> DIARY = [] {
		std::array<DiaryText, DIARY_PAGE_COUNT> diary{};
		for (size_t p = 0; p < diary.size(); p++) {
			const std::string key = std::string("diary.") + SLUGS[p];
			diary[p].title = gameText(key + ".title").c_str();
			for (size_t i = 0; i < diary[p].paragraphs.size(); i++) {
				const std::string* paragraph = gameTexts().Find(key + ".p" + std::to_string(i + 1));
				diary[p].paragraphs[i] = paragraph == nullptr ? nullptr : paragraph->c_str();
			}
			if (diary[p].paragraphs[0] == nullptr)
				diary[p].paragraphs[0] = gameText(key + ".p1").c_str();
		}
		return diary;
	}();
	return DIARY[static_cast<size_t>(page)];
}

const ExpeditionPageText& expeditionPageText(int number) {
	static const std::array<ExpeditionPageText, EXPEDITION_PAGE_COUNT> PAGES = [] {
		std::array<ExpeditionPageText, EXPEDITION_PAGE_COUNT> pages{};
		for (size_t n = 0; n < pages.size(); n++) {
			const std::string key = "page." + std::to_string(n + 1);
			pages[n] = {gameText(key + ".heading").c_str(), gameText(key + ".text").c_str()};
		}
		return pages;
	}();
	return PAGES[static_cast<size_t>(std::clamp(number, 1, EXPEDITION_PAGE_COUNT) - 1)];
}
