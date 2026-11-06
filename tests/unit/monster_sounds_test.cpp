// Monster sounds (plan missing-monster-sounds): every sound the code can play for a kind has its file, under the kind's
// own voice (a boss's) or its model's.
#include "../../external/doctest/doctest.h"
#include "../../src/core/paths.h"
#include "../../src/level/monster_kinds.h"
#include "../../src/level/world_events.h"
#include <filesystem>
#include <string>

TEST_CASE("every sound a monster kind can play has a file") {
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) {
		const MonsterKind& kind = *monsterKind(id);
		const std::string voice = *kind.sounds != '\0' ? kind.sounds : kind.model;
		for (int i = 0; i < CHARACTER_SOUND_COUNT; i++) {
			const auto sound = static_cast<CharacterSound>(i);
			const std::string path = MEDIA_DIR "sounds/" + voice + characterSoundSuffix(sound);
			CAPTURE(path);
			if (playsSound(kind, sound))
				CHECK(std::filesystem::exists(path));
		}
	}
}

TEST_CASE("which sounds a kind can play: its abilities and its way of lurking") {
	CHECK(playsSound(*monsterKind(MonsterPlant), CharacterSound::Attack));
	CHECK_FALSE(playsSound(*monsterKind(MonsterPlant), CharacterSound::Wake));
	CHECK(playsSound(*monsterKind(MonsterWorm), CharacterSound::Wake)); // surfacing from the sand
	CHECK(playsSound(*monsterKind(MonsterGiantScorpion), CharacterSound::Wake)); // the grab's tell
	CHECK_FALSE(playsSound(*monsterKind(MonsterScorpion), CharacterSound::Wake));
	CHECK(playsSound(*monsterKind(MonsterGiantScarab), CharacterSound::Jump));
	CHECK(playsSound(*monsterKind(MonsterGiantScarab), CharacterSound::AttackRanged)); // the dung ball
	CHECK_FALSE(playsSound(*monsterKind(MonsterScarab), CharacterSound::Jump));
	CHECK(playsSound(*monsterKind(MonsterGiantRat), CharacterSound::Roar)); // the pack call
}
