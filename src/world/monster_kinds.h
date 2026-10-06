#ifndef MONSTER_KINDS_H
#define MONSTER_KINDS_H

// One row per monster type, the facts without the model: name, glyph, the checker's threat, boss or not.
// The game's stats and models are MONSTER_DEFS / BOSS_DEFS (state/assets.cpp), by the same MonsterTypeId.

#include "level.h"

struct MonsterKind {
	MonsterTypeId id;
	const char* label;		 // the editor's list ("giant rat")
	const char* description; // the editor's hint
	char glyph;				 // levelcheck --map and the ASCII level sources
	float threat;			 // one of them in the checker's difficulty score
	bool boss;				 // summons minions, its death opens the boss gates (BOSS_LOCK); at most one per level
	const char* note;		 // the journal's description, in the archaeologist's words, written after the first kill
};

// nullptr for an unknown type.
[[nodiscard]] const MonsterKind* monsterKind(int type);
[[nodiscard]] inline bool isBossMonster(int type) {
	const MonsterKind* kind = monsterKind(type);
	return kind != nullptr && kind->boss;
}

// Its bite or sting poisons the player (the tier: POISON_DEFS in state/assets.cpp). The checker wants an antidote in
// reach on a level with one (docs/plan/poison-and-antidote.md).
[[nodiscard]] bool isPoisoner(int type);

constexpr char UNKNOWN_MONSTER_GLYPH = 'm';
constexpr float UNKNOWN_MONSTER_THREAT = 2.f;

#endif
