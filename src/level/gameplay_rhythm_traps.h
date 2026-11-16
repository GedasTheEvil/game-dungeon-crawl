#ifndef GAMEPLAY_RHYTHM_TRAPS_H
#define GAMEPLAY_RHYTHM_TRAPS_H

#include "gameplay_player.h"
#include "gameplay_shared.h"

// Wall spikes (plan rhythm-traps): out of the back wall for SPIKES_OUT_MS of each SPIKES_PERIOD_MS, by speed (normal,
// fast, slow, SpikeSpeed); the last SPIKES_TELEGRAPH_MS before they come out they twitch and scrape. Out, they block
// like a closed gate; whoever stands in the cell when they come out takes one stab of SPIKES_STAB_DAMAGE and weak
// poison, and is shoved out of it. The safe window lets the player walk through the cell with time to spare.
constexpr int SPIKES_PERIOD_MS[3] = {4000, 3200, 4800};
constexpr int SPIKES_OUT_MS[3] = {1200, 800, 1600};
constexpr int SPIKES_TELEGRAPH_MS = 600;
constexpr int SPIKES_STAB_DAMAGE = 10;
constexpr float SPIKES_CROSS_MS = (1.f + 2.f * PLAYER_BODY_HALF_WIDTH) / WALK_SPEED * 1000.f; // walking through
static_assert(
	static_cast<float>(SPIKES_PERIOD_MS[0] - SPIKES_OUT_MS[0]) >= 1.5f * SPIKES_CROSS_MS &&
		static_cast<float>(SPIKES_PERIOD_MS[1] - SPIKES_OUT_MS[1]) >= 1.5f * SPIKES_CROSS_MS &&
		static_cast<float>(SPIKES_PERIOD_MS[2] - SPIKES_OUT_MS[2]) >= 1.5f * SPIKES_CROSS_MS,
	"a single wall spike's safe window is 1.5 times the walk through it"
);

// Sand gusts (plan sandstorm-halls, SandGust): a gust blows for GUST_OUT_MS of each GUST_PERIOD_MS, by speed (normal,
// fast, slow, SpikeSpeed); for the last GUST_TELEGRAPH_MS before it the hiss rises and grit skitters along the floor.
// It pushes the player in its cell downwind at GUST_PUSH_SPEED, faster than the walk: against the wind they lose
// ground, so they cross between gusts and wait them out in a shelter (a cell of the hall without a gust). A light
// monster (weight up to GUST_LIGHT_WEIGHT: the small ones and the flyers) goes at GUST_MONSTER_PUSH_SPEED; arrows and
// spit drift GUST_DRIFT_SPEED along their flight. Near a hall (GUST_HAZE_REACH along the row) the haze closes in, at
// GUST_HAZE_CALM between gusts, and in a gust the wind takes GUST_MUFFLE of the other sounds' volume. The checker wants
// a run of gusts crossed against the wind in one calm, GUST_CROSS_MARGIN_MS to spare.
constexpr int GUST_PERIOD_MS[3] = {7000, 5600, 8400};
constexpr int GUST_OUT_MS[3] = {2000, 1600, 2400};
constexpr int GUST_TELEGRAPH_MS = 1200;
constexpr float GUST_PUSH_SPEED = 1.6f; // tiles per second
constexpr int GUST_LIGHT_WEIGHT = 1;
constexpr float GUST_MONSTER_PUSH_SPEED = 1.2f;
constexpr float GUST_DRIFT_SPEED = 2.5f;
constexpr float GUST_HAZE_REACH = 4.f;
constexpr int GUST_HAZE_FADE_MS = 600; // the haze follows the storm's strength this slowly
constexpr float GUST_HAZE_CALM = 0.25f;
constexpr float GUST_MUFFLE = 0.65f;
constexpr int GUST_CROSS_MARGIN_MS = 300;

#endif
