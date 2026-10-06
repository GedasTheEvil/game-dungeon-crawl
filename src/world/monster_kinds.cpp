#include "monster_kinds.h"
#include <array>

namespace {
constexpr std::array<MonsterKind, MONSTER_TYPE_MAX> KINDS = {{
	{MonsterScarab, "scarab", "Scarab", 's', 0.8f, false, "A beetle the size of my fist. Its bite stings."},
	{MonsterWorm, "worm", "Worm", 'w', 2.f, false, "Slow as sand, but its jaws close like a vice."},
	// Threat: does not move.
	{MonsterPlant, "plant", "Plant", 'p', 1.5f, false, "Rooted to the spot. Never stand next to it."},
	{MonsterAnubis, "anubis", "Anubis", 'n', 8.f, false, "A jackal-headed guard. Traps do not stop him."},
	// Threat: fast, but barely hurts.
	{MonsterRat, "rat", "Rat", 't', 0.7f, false, "Quick, filthy and always hungry."},
	{MonsterGiantRat, "giant rat", "Giant rat", 'T', 3.f, false, "A rat as big as a dog. Jumps at me over the pits."},
	// Threat: weak, but only hit with good timing.
	{MonsterBat, "bat", "Bat", 'f', 1.2f, false, "Flits down out of the dark. Hard to hit in the air."},
	{MonsterGiantBat, "giant bat", "Giant bat", 'F', 3.5f, false, "Its wings are wider than my arms."},
	// Threat: hits hard, but does not move.
	{MonsterMimic, "mimic", "Mimic, a treasure chest until the player comes near", 'M', 2.f, false,
	 "Not every chest in this tomb holds treasure."},
	// Threat: jumps like a giant rat, hits harder.
	{MonsterGiantScarab, "giant scarab", "Giant scarab, leaps over pits and traps", 'k', 4.f, false,
	 "A shell like a shield, and it jumps."},
	// Threat: a little weaker than an Anubis, plus its scarabs.
	{MonsterBossScarab, "boss scarab",
	 "Boss scarab, summons scarabs; its death opens the boss gates. One boss per level", 'K', 10.f, true,
	 "The mother of the scarabs. Her brood comes when she calls."},
	// Threat: a giant bat that hits like a boss and heals, plus its bats.
	{MonsterVampireBat, "vampire bat",
	 "Vampire bat, summons bats and heals by part of the damage it deals; its death opens the boss gates. One boss per "
	 "level",
	 'V', 12.f, true, "Grown fat on blood. Every bite makes it stronger."},
	// Threat: walks fast for its size and hits hard, but gives the player time to see it climb out.
	{MonsterMummy, "mummy", "Mummy, lies in its coffin until the player comes near", 'u', 5.f, false,
	 "Wrapped and patient. It waited three thousand years for me."},
	// Threat: the finale. Four hits kill a level 30 player, plus its mummies.
	{MonsterAnubisBoss, "anubis boss",
	 "Anubis boss, mummies climb out of the coffins round it; its death opens the boss gates. One boss per level", 'N',
	 15.f, true, "The guardian of the last tomb. The dead rise at his word."},
	// Threat: between the giant rat and the mummy, bites harder than both; in the water it outswims the player.
	{MonsterCrocodile, "crocodile", "Crocodile, lies under the water until the player comes near, swims fast", 'C',
	 4.5f, false, "Sobek's own. In the water I cannot outrun it."},
	// Threat: a little above the rat and the scarab: the poison burns on after the fight.
	{MonsterScorpion, "scorpion", "Scorpion, its sting poisons (weak)", 'j', 1.f, false,
	 "Pale as straw, quick on its eight legs. The sting burns for a long while."},
	// Threat: a giant rat's, the poison on top, and it spits from afar.
	{MonsterCobra, "cobra", "Cobra, lies coiled until the player comes near; its bite and spit poison (medium)", 'c',
	 3.f, false, "The uraeus of the crowns, alive. It spits before it bites, and both burn."},
	// Threat: between the giant scarab and the crocodile, the poison and the spit on top.
	{MonsterGiantCobra, "giant cobra",
	 "Giant cobra, lies coiled until the player comes near; its bite and spit poison "
	 "(medium)",
	 'Q', 5.f, false, "Black as the night of Apep, long as a boat. It spits farther than I can jump."},
	// Threat: a giant rat's, the poison on top.
	{MonsterGiantScorpion, "giant scorpion", "Giant scorpion, its sting poisons (medium)", 'J', 3.5f, false,
	 "Dark as old blood and as big as a dog. Its sting does not wear off quickly."},
}};
} // namespace

bool isPoisoner(int type) {
	return type == MonsterScorpion || type == MonsterCobra || type == MonsterGiantCobra || type == MonsterGiantScorpion;
}

const MonsterKind* monsterKind(int type) {
	for (const MonsterKind& kind : KINDS)
		if (kind.id == type)
			return &kind;
	return nullptr;
}
