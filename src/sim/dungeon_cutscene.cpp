// Cut-scenes (plan cinematic-effects): a lever or a boss's death opens gates off the screen; the camera's way is the
// CutScene's (cut_scene.cpp). Here: each gate starts to open as the camera comes to it, and opens at once on a skip.
#include "dungeon.h"
#include "../core/timer.h"
#include <algorithm>

void Dungeon::startCutScene(const std::vector<int>& opened) {
	if (Travelling())
		return;
	const std::vector<CutScene::Stop> stops = cutScene.Start(opened, view(), mapX, mapY);
	if (stops.empty())
		return;
	for (const CutScene::Stop& stop : stops)
		for (Motion& m : openingGates)
			if (m.cell == stop.cell)
				m.startMs = stop.arrivalMs;
	updateCutScene();
}
void Dungeon::updateCutScene() {
	finishOpenedGates();
	if (!cutScene.Update(mapX, mapY))
		SkipCutScene();
}
void Dungeon::SkipCutScene() {
	const int now = GameClock::now();
	for (Motion& m : openingGates) // a gate the camera had not come to yet opens now
		m.startMs = std::min(m.startMs, now);
	cutScene.End();
}
