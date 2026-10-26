#include "journal_text.h"
#include "items.h"
#include "monster_kinds.h"
#include "texts.h"
#include <algorithm>
#include <array>

namespace {

const char* moveText(const char* move, const char* variant = "text") {
	return gameText(std::string("move.") + move + "." + variant).c_str();
}

// The curse its hits lay, the first one listed (melee first). The mummy priest's are too many to name.
const char* curseMoveNote(const MonsterKind& t) {
	if (t.id == MonsterMummyPriest)
		return moveText("curse", "priest");
	const CurseEntry& e = t.meleeCurses[0].percent > 0 ? t.meleeCurses[0] : t.rangedCurses[0];
	return gameText("move.curse." + textSlug(curseDef(e.kind).name)).c_str();
}

} // namespace

const char* creatureMoveNote(CreatureMove move, const MonsterKind& t) {
	switch (move) {
	case CreatureMove::Leap:
		return moveText("leap");
	case CreatureMove::Swoop:
		return moveText("swoop");
	case CreatureMove::Ambush:
		return moveText("ambush");
	case CreatureMove::Rise:
		return moveText("rise");
	case CreatureMove::Summon:
		switch (t.boss.summon) {
		case Summon::DigOut:
			return moveText("summon", t.id == MonsterBabi ? "babi" : "dig-out");
		case Summon::Drop:
			return moveText("summon", "drop");
		case Summon::Coffin:
			return moveText("summon", "coffin");
		case Summon::Hatch:
			return moveText("summon", "hatch");
		case Summon::Wall:
			return moveText("summon", t.boss.smallMinion != 0 ? "wall-servants" : "wall");
		}
		break;
	case CreatureMove::Heal:
		return moveText("heal");
	case CreatureMove::Surface:
		return moveText("surface");
	case CreatureMove::Poison:
		return moveText("poison");
	case CreatureMove::Rear:
		return moveText("rear");
	case CreatureMove::AttackRanged:
		if (t.id == MonsterMummyPriest)
			return moveText("attack-ranged", "priest");
		if (t.id == MonsterBaboon)
			return moveText("attack-ranged", "baboon");
		if (t.ranged && t.ranged->volley)
			return moveText("attack-ranged", "volley");
		return moveText("attack-ranged", t.ranged && t.ranged->burns() ? "fire" : "venom");
	case CreatureMove::Burrow:
		if (t.sandBurrow && t.sandBurrow->behind)
			return moveText("burrow", "sand-behind");
		if (t.sandBurrow)
			return moveText("burrow", "sand");
		return moveText("burrow");
	case CreatureMove::Charge:
		return moveText("charge");
	case CreatureMove::Climb:
		return moveText("climb");
	case CreatureMove::Curse:
		return curseMoveNote(t);
	case CreatureMove::Bloom:
		return moveText("bloom");
	case CreatureMove::Sedate:
		return moveText("sedate");
	case CreatureMove::ShunLight:
		return moveText("shun-light");
	case CreatureMove::Grab:
		if (t.hold && t.hold->sting)
			return moveText("grab", "sting");
		if (t.hold && t.hold->swallow)
			return moveText("grab", "swallow");
		return moveText("grab");
	case CreatureMove::StepOff:
		return moveText("step-off");
	case CreatureMove::Roar:
		return moveText("roar");
	case CreatureMove::Kite:
		return moveText("kite");
	case CreatureMove::RollBall:
		return moveText("roll-ball", t.roll && t.roll->burns() ? "burning" : "text");
	case CreatureMove::Douse:
		return moveText("douse");
	case CreatureMove::SpinWeb:
		return moveText("spin-web");
	case CreatureMove::Blaze:
		return moveText("blaze");
	case CreatureMove::LeapAway:
		return moveText("leap-away");
	case CreatureMove::Blind:
		return moveText("blind", t.ranged && t.ranged->fan > 1 ? "fan" : "text");
	}
	return "";
}

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

std::string fieldNoteKey(int note) {
	const int amulet = note - static_cast<int>(FieldNote::AmuletFirst);
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
	static constexpr std::array<const char*, DIARY_PAGE_COUNT> SLUGS = {"arrival", "descent"};
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
