// Cut-scenes (plan cinematic-effects): the camera eases to each gate (at most CUTSCENE_MAX_STOPS, nearest first),
// holds while it opens, and eases back to the player. The look (bars, tint) is drawn by the game from the shot.
#include "cut_scene.h"
#include "../../core/timer.h"
#include "../../level/tuning/gameplay_world.h"
#include "../../level/level.h"
#include <algorithm>
#include <utility>

namespace {
float smooth(float t) { return t * t * (3.f - 2.f * t); }

// The cell's middle from (x, y), in tiles.
std::pair<float, float> offset(int cell, float x, float y) {
	const int row = cell / LEVEL_WIDTH;
	return {static_cast<float>(cell % LEVEL_WIDTH) + 0.5f - x, static_cast<float>(row) - y};
}
} // namespace

int CutScene::lengthMs() const {
	const auto count = static_cast<int>(stops.size());
	return (count + 1) * CUTSCENE_PAN_MS + count * CUTSCENE_HOLD_MS;
}
std::vector<CutScene::Stop> CutScene::Start(const std::vector<int>& opened, ViewWindow view, float x, float y) {
	if (!enabled || Playing())
		return {};
	std::vector<int> remote;
	for (int cell : opened)
		if (!view.contains(static_cast<float>(cell % LEVEL_WIDTH) + 0.5f, cell / LEVEL_WIDTH))
			remote.push_back(cell);
	if (remote.empty())
		return {};
	auto distance = [x, y](int cell) {
		const auto [dx, dy] = offset(cell, x, y);
		return dx * dx + dy * dy;
	};
	std::stable_sort(remote.begin(), remote.end(), [&](int a, int b) { return distance(a) < distance(b); });
	if (remote.size() > static_cast<size_t>(CUTSCENE_MAX_STOPS))
		remote.resize(CUTSCENE_MAX_STOPS);

	stops = remote;
	startMs = GameClock::now();
	std::vector<Stop> shown;
	shown.reserve(stops.size());
	for (size_t k = 0; k < stops.size(); k++)
		shown.push_back(
			{stops[k], startMs + static_cast<int>(k + 1) * CUTSCENE_PAN_MS + static_cast<int>(k) * CUTSCENE_HOLD_MS}
		);
	return shown;
}
bool CutScene::Update(float x, float y) {
	const int t = GameClock::now() - startMs;
	const int length = lengthMs();
	if (t >= length)
		return false;
	// The camera's way: the player, each gate, the player again; a pan into each point, a hold on each gate.
	auto point = [&](size_t k) -> std::pair<float, float> { // k: 0 and stops + 1 the player, 1..stops the gates
		if (k == 0 || k > stops.size())
			return {0.f, 0.f};
		return offset(stops[k - 1], x, y);
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
	shot = {
		x0 + (x1 - x0) * k,
		y0 + (y1 - y0) * k,
		smooth(std::clamp(static_cast<float>(edge) / static_cast<float>(CUTSCENE_BARS_MS), 0.f, 1.f))
	};
	return true;
}
void CutScene::End() {
	stops.clear();
	startMs = 0;
	shot = {};
}
