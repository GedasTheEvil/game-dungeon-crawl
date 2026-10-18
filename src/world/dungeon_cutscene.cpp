// Cut-scenes (plan cinematic-effects): a lever or a boss's death opens gates off the screen; the camera eases to each
// (at most CUTSCENE_MAX_STOPS, nearest first), holds while it opens, and eases back to the player. The world stands
// still meanwhile, only the gates move. The look (bars, tint) is drawn by the game from CutSceneShot.
#include "dungeon.h"
#include "dungeon_rules.h"
#include "../core/gameplay_config.h"
#include <algorithm>

namespace {
float smooth(float t) { return t * t * (3.f - 2.f * t); }
} // namespace

int Dungeon::cutSceneLengthMs() const {
	const auto stops = static_cast<int>(cutScene.stops.size());
	return (stops + 1) * CUTSCENE_PAN_MS + stops * CUTSCENE_HOLD_MS;
}
//======================================================================================
void Dungeon::startCutScene(const std::vector<int>& opened) {
	if (!cutScenesOn || CutScenePlaying() || Travelling())
		return;
	const ViewWindow v = view();
	std::vector<int> remote;
	for (int cell : opened)
		if (!v.contains(static_cast<float>(cell % MAP_WIDTH) + 0.5f, cell / MAP_WIDTH))
			remote.push_back(cell);
	if (remote.empty())
		return;
	auto distance = [this](int cell) {
		const float dx = static_cast<float>(cell % MAP_WIDTH) + 0.5f - mapX;
		const int row = cell / MAP_WIDTH;
		const float dy = static_cast<float>(row) - mapY;
		return dx * dx + dy * dy;
	};
	std::stable_sort(remote.begin(), remote.end(), [&](int a, int b) { return distance(a) < distance(b); });
	if (remote.size() > static_cast<size_t>(CUTSCENE_MAX_STOPS))
		remote.resize(CUTSCENE_MAX_STOPS);

	cutScene = {remote, GameClock::now()};
	for (size_t k = 0; k < remote.size(); k++) { // each gate starts to open as the camera comes to it
		const int arrival =
			cutScene.startMs + static_cast<int>(k + 1) * CUTSCENE_PAN_MS + static_cast<int>(k) * CUTSCENE_HOLD_MS;
		for (Motion& m : openingGates)
			if (m.cell == remote[k])
				m.startMs = arrival;
	}
	updateCutScene();
}
//======================================================================================
void Dungeon::updateCutScene() {
	finishOpenedGates();
	const int t = GameClock::now() - cutScene.startMs;
	const int length = cutSceneLengthMs();
	if (t >= length) {
		SkipCutScene();
		return;
	}
	// The camera's way: the player, each gate, the player again; a pan into each point, a hold on each gate.
	auto point = [this](size_t k) -> std::pair<float, float> { // k: 0 and stops + 1 the player, 1..stops the gates
		if (k == 0 || k > cutScene.stops.size())
			return {0.f, 0.f};
		const int cell = cutScene.stops[k - 1];
		const int row = cell / MAP_WIDTH;
		return {static_cast<float>(cell % MAP_WIDTH) + 0.5f - mapX, static_cast<float>(row) - mapY};
	};
	int left = t;
	size_t from = 0;
	float k = 0.f; // how far from `from` to the next point
	while (true) {
		if (left < CUTSCENE_PAN_MS) {
			k = smooth(static_cast<float>(left) / static_cast<float>(CUTSCENE_PAN_MS));
			break;
		}
		left -= CUTSCENE_PAN_MS;
		from++;
		if (left < CUTSCENE_HOLD_MS) {
			k = 0.f;
			break;
		}
		left -= CUTSCENE_HOLD_MS;
	}
	const auto [x0, y0] = point(from);
	const auto [x1, y1] = point(from + 1);
	const int edge = std::min(t, length - t);
	cutShot = {x0 + (x1 - x0) * k, y0 + (y1 - y0) * k,
			   smooth(std::clamp(static_cast<float>(edge) / static_cast<float>(CUTSCENE_BARS_MS), 0.f, 1.f))};
}
//======================================================================================
void Dungeon::SkipCutScene() {
	const int now = GameClock::now();
	for (Motion& m : openingGates) // a gate the camera had not come to yet opens now
		m.startMs = std::min(m.startMs, now);
	cutScene = {};
	cutShot = {};
}
