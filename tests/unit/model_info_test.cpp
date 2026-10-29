#include "../../external/doctest/doctest.h"
#include "../../src/core/gameplay_config.h"
#include "../../src/core/timer.h"
#include "../../src/entities/model_info.h"
#include "../../src/world/level.h"
#include "../../src/world/monster_kinds.h"
#include "../../src/world/rng.h"
#include <algorithm>
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

TEST_CASE("a model draws the step under way blended between two key frames") {
	// A loop of 4 frames at 1 frame a step: frame 2 at 0 ms, the next step sets frame 3.
	const AnimPlayback p{2.f, 0, 1.f};
	const FrameBlend start = BlendOf(p, 4, true, 0, 1000);
	CHECK(start.a == 2);
	CHECK(start.b == 3);
	CHECK(start.t == doctest::Approx(0.f));
	const FrameBlend half = BlendOf(p, 4, true, FRAME_STEP_PERIOD_MS / 2, 1000);
	CHECK(half.a == 2);
	CHECK(half.t == doctest::Approx(0.5f));
	// Past the step not taken yet (a late tick): held at what it will set.
	const FrameBlend late = BlendOf(p, 4, true, 3 * FRAME_STEP_MS, 1000);
	CHECK(late.a == 3);
	CHECK(late.t == doctest::Approx(0.f));

	SUBCASE("a loop goes on from its last frame to frame 0") {
		const AnimPlayback end{3.f, 0, 1.f};
		const FrameBlend f = BlendOf(end, 4, true, FRAME_STEP_PERIOD_MS / 4, 1000);
		CHECK(f.a == 3);
		CHECK(f.b == 0);
		CHECK(f.t == doctest::Approx(0.25f));
		CHECK(BlendOf(end, 4, true, FRAME_STEP_PERIOD_MS, 1000).a == 0);
	}
	SUBCASE("a one-shot clip holds its last frame") {
		const AnimPlayback nearEnd{2.5f, 0, 1.f}; // the next step clamps to 3
		const FrameBlend f = BlendOf(nearEnd, 4, false, FRAME_STEP_PERIOD_MS / 2, 1000);
		CHECK(f.a == 2);
		CHECK(f.b == 3);
		CHECK(f.t == doctest::Approx(0.75f));
		const FrameBlend held = BlendOf({3.f, 0, 1.f}, 4, false, FRAME_STEP_PERIOD_MS / 2, 1000);
		CHECK(held.a == 3);
		CHECK(held.b == 3);
		CHECK(held.t == 0.f);
	}
	SUBCASE("a frame set by hand (a pose, the climb) blends its fraction, the clock aside") {
		const FrameBlend f = BlendOf({1.25f, 0, 0.f}, 4, true, 3 * FRAME_STEP_MS, 1000);
		CHECK(f.a == 1);
		CHECK(f.b == 2);
		CHECK(f.t == doctest::Approx(0.25f));
		const FrameBlend wrap = BlendOf({3.5f, 0, 0.f}, 4, true, 0, 1000);
		CHECK(wrap.a == 3);
		CHECK(wrap.b == 0);
	}
	SUBCASE("the animation rate snaps the clock; key frames draw the stepped frame") {
		// 24 poses a second: 70 ms shows the pose of 41.7 ms.
		CHECK(BlendOf(p, 4, true, 70, 24).t == doctest::Approx(1000.f / 24.f / FRAME_STEP_PERIOD_MS));
		const FrameBlend keys = BlendOf({2.6f, 0, 1.f}, 4, true, FRAME_STEP_MS / 2, 0);
		CHECK(keys.a == 2);
		CHECK(keys.b == 2);
		CHECK(keys.t == 0.f);
	}
	SUBCASE("t stays in [0, 1) as a clip plays") {
		GameClock::enableVirtual();
		AnimPlayback q;
		q.stepStart = GameClock::now();
		for (int ms = 0; ms < 3000; ms += 7) {
			GameClock::advance(7);
			AdvancePlayback(q, 5, true, ModelInfo::CLIP_SPEED);
			const FrameBlend f = BlendOf(q, 5, true, GameClock::now(), 60);
			CHECK(f.t >= 0.f);
			CHECK(f.t < 1.f);
			CHECK(f.a >= 0);
			CHECK(f.a < 5);
		}
	}
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

TEST_CASE("the walk clip plays as many rounds a second as asked; its attack clip at the usual pace") {
	GameClock::enableVirtual();
	ModelInfo info;
	info.clips[static_cast<int>(ModelState::Move)] = {true, 26, true};
	info.clips[static_cast<int>(ModelState::Attack)] = {true, 26, true};
	CHECK(info.WalkSpeed(0.5f) == doctest::Approx(info.WalkSpeed(1.f) / 2.f));
	for (const ModelState state : {ModelState::Move, ModelState::Attack}) {
		ClipPlayback playback{};
		playback[static_cast<int>(state)].stepStart = GameClock::now();
		for (int t = 0; t < 5; t++) { // 0.56 s: five steps on the game's ticks
			GameClock::advance(FRAME_STEP_PERIOD_MS);
			info.Advance(state, playback, 0, info.WalkSpeed(2.2f));
		}
		const float frame = playback[static_cast<int>(state)].frame;
		if (state == ModelState::Move) // 1.232 rounds: on through the loop's end, not snapped to frame 0
			CHECK(frame == doctest::Approx(0.232f * 26.f));
		else
			CHECK(frame == doctest::Approx(0.04f * ModelInfo::CLIP_SPEED * 5.f));
	}
}

TEST_CASE("a fast walk clip plays on smoothly on the game's ticks, no hold before each step") {
	GameClock::enableVirtual();
	ModelInfo info;
	info.clips[static_cast<int>(ModelState::Move)] = {true, 26, true};
	ClipPlayback playback{};
	AnimPlayback& p = playback[static_cast<int>(ModelState::Move)];
	p.stepStart = GameClock::now();
	const float speed = info.WalkSpeed(3.3f); // the Anubis guard's legs: 8.6 frames a step
	float last = -1.f, least = 1e9f, most = 0.f;
	for (int t = 0; t < 3000; t += UPDATE_TICK_MS) {
		GameClock::advance(UPDATE_TICK_MS);
		info.Advance(ModelState::Move, playback, 0, speed);
		const FrameBlend f = BlendOf(p, 26, true, GameClock::now(), 1000);
		const float pos = static_cast<float>(f.a) + f.t;
		if (last >= 0.f) {
			const float d = pos >= last ? pos - last : pos + 26.f - last;
			least = std::min(least, d);
			most = std::max(most, d);
		}
		last = pos;
	}
	CHECK(least > 0.5f * most); // every tick moves the legs on about as far
}
