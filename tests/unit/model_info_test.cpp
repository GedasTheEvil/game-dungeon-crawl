#include "../../external/doctest/doctest.h"
#include "../../src/core/timer.h"
#include "../../src/entities/model_info.h"
#include "../../src/world/level.h"
#include "../../src/world/monster_kinds.h"
#include "../../src/world/rng.h"
#include <string>

TEST_CASE("every monster model has its clips, measured round its centre") {
	for (int id = 1; id <= MONSTER_TYPE_MAX; id++) {
		const MonsterKind& kind = *monsterKind(id);
		CAPTURE(std::string(kind.model));
		ModelInfo info;
		REQUIRE(LoadClips(kind.model, ClipFilesOf(kind), info));
		for (const ClipFile& file : ClipFilesOf(kind)) {
			const ClipInfo& clip = info.Clip(file.state);
			if (file.required)
				CHECK(clip.present);
			if (clip.present) {
				CHECK(clip.frames >= 1);
				CHECK(clip.loop == file.loop);
			}
		}
		// Centrify: the largest extent is 1, the base at 0, centred in x and z.
		CHECK(info.referenceTop > 0.f);
		CHECK(info.referenceTop <= 1.001f);
		CHECK(info.HalfWidth() > 0.f);
		CHECK(info.HalfWidth() <= 1.001f);
		if (info.Clip(ModelState::Idle).present)
			CHECK(info.idleBottom < info.idleTop);
	}
}

TEST_CASE("the player model has its clips and stands on its base") {
	ModelInfo info;
	REQUIRE(LoadClips("characters/archeologist", PLAYER_CLIPS, info));
	CHECK(info.reference == ModelState::Idle);
	CHECK(info.Clip(ModelState::Move).present);
	CHECK(info.Clip(ModelState::Die).present);
	CHECK_FALSE(info.Clip(ModelState::Attack).present);
	CHECK(info.Shown(ModelState::Attack) == ModelState::Idle); // no attack clip: the weapon swings
	CHECK(info.idleBottom == doctest::Approx(0.f).epsilon(0.001));
	CHECK(info.idleTop == doctest::Approx(info.referenceTop));
}

TEST_CASE("a one-shot clip plays from its start once and holds its last frame") {
	GameClock::enableVirtual();
	ModelInfo info;
	info.reference = ModelState::Move;
	info.clips[static_cast<int>(ModelState::Move)] = {true, 10, true};
	info.clips[static_cast<int>(ModelState::Die)] = {true, 5, false};
	ClipPlayback playback{};
	for (AnimPlayback& p : playback)
		p.stepStart = GameClock::now();
	ModelState state = ModelState::Move;

	playback[static_cast<int>(ModelState::Die)].frame = 3.f;
	info.Enter(state, ModelState::Die, playback);
	CHECK(state == ModelState::Die);
	CHECK(playback[static_cast<int>(ModelState::Die)].frame == 0.f);
	CHECK(info.Progress(state, playback) == 0.f);

	for (int t = 0; t < 100 && !info.Finished(state, playback); t++) {
		GameClock::advance(FRAME_STEP_MS);
		info.Advance(state, playback);
	}
	CHECK(info.Finished(state, playback));
	CHECK(info.Progress(state, playback) == 1.f);
	GameClock::advance(FRAME_STEP_MS);
	info.Advance(state, playback);
	CHECK(playback[static_cast<int>(ModelState::Die)].frame == 4.f);

	// Entering the state it is in does not restart it.
	info.Enter(state, ModelState::Die, playback);
	CHECK(info.Finished(state, playback));
	// A state without its own clip shows the reference clip.
	info.Enter(state, ModelState::Attack, playback);
	CHECK(info.Shown(state) == ModelState::Move);
}

TEST_CASE("a looping clip starts over") {
	GameClock::enableVirtual();
	AnimPlayback p;
	p.stepStart = GameClock::now();
	float last = 0.f;
	bool wrapped = false;
	for (int t = 0; t < 100; t++) {
		GameClock::advance(FRAME_STEP_MS);
		AdvancePlayback(p, 4, true, ModelInfo::CLIP_SPEED);
		CHECK(p.frame < 4.f);
		wrapped = wrapped || p.frame < last;
		last = p.frame;
	}
	CHECK(wrapped);
}

TEST_CASE("monsters spawned together start their walk at different frames") {
	ModelInfo info;
	info.clips[static_cast<int>(ModelState::Move)] = {true, 20, true};
	Rng rng(7);
	const ClipPlayback a = info.SpawnPlayback(rng);
	const ClipPlayback b = info.SpawnPlayback(rng);
	CHECK(a[static_cast<int>(ModelState::Move)].frame != b[static_cast<int>(ModelState::Move)].frame);
	CHECK(a[static_cast<int>(ModelState::Idle)].frame == 0.f); // no idle clip
}
