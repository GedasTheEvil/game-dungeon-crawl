// Cut-scenes (plan cinematic-effects): a lever or a boss's death opens gates off the screen; the camera's way is the
// CutScene's (cut_scene.cpp). Here: each gate starts to open as the camera comes to it, and opens at once on a skip.
#include "dungeon.h"
#include "../../core/timer.h"

void Dungeon::startCutScene(const std::vector<int>& opened) {
	if (Travelling())
		return;
	const std::vector<CutScene::Stop> stops = cutScene.Start(opened, view(), mapX, mapY);
	if (stops.empty())
		return;
	for (const CutScene::Stop& stop : stops)
		gates.DelayOpening(stop.cell, stop.arrivalMs);
	updateCutScene();
}
void Dungeon::updateCutScene() {
	gates.FinishOpened(map);
	if (!cutScene.Update(mapX, mapY))
		SkipCutScene();
}
void Dungeon::SkipCutScene() {
	gates.HurryOpening(GameClock::now()); // a gate the camera had not come to yet opens now
	cutScene.End();
}
