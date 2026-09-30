#include "monster_kinds.h"
#include <array>

namespace {
constexpr std::array<MonsterKind, MONSTER_TYPE_MAX> KINDS = {{
	{MonsterScarab, "scarab", "Scarab", 's', 0.8f, false},
	{MonsterWorm, "worm", "Worm", 'w', 2.f, false},
	// Threat: does not move.
	{MonsterPlant, "plant", "Plant", 'p', 1.5f, false},
	{MonsterAnubis, "anubis", "Anubis", 'n', 8.f, false},
	// Threat: fast, but barely hurts.
	{MonsterRat, "rat", "Rat", 't', 0.7f, false},
	{MonsterGiantRat, "giant rat", "Giant rat", 'T', 3.f, false},
	// Threat: weak, but only hit with good timing.
	{MonsterBat, "bat", "Bat", 'f', 1.2f, false},
	{MonsterGiantBat, "giant bat", "Giant bat", 'F', 3.5f, false},
	// Threat: hits hard, but does not move.
	{MonsterMimic, "mimic", "Mimic, a treasure chest until the player comes near", 'M', 2.f, false},
	// Threat: jumps like a giant rat, hits harder.
	{MonsterGiantScarab, "giant scarab", "Giant scarab, leaps over pits and traps", 'k', 4.f, false},
	// Threat: a little weaker than an Anubis, plus its scarabs.
	{MonsterBossScarab, "boss scarab",
	 "Boss scarab, summons scarabs; its death opens the boss gates. One boss per level", 'K', 10.f, true},
}};
} // namespace

const MonsterKind* monsterKind(int type) {
	for (const MonsterKind& kind : KINDS)
		if (kind.id == type)
			return &kind;
	return nullptr;
}
