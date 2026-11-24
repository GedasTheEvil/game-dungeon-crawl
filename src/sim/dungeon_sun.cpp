// Sunlight mirrors (plan sunlight-mirrors): the beams of the light shafts, the mirrors that turn them, the sun discs
// they light. The beams' state is SunMirrors' (mechanisms.h), the tracing sun_beam.h's, shared with the checker; drawn
// in dungeon_render_sun.cpp.
#include "dungeon.h"
#include "dungeon_rules.h"
#include "../level/rules/world_events.h"
#include "../level/tuning/gameplay_world.h"
#include <algorithm>
#include <cmath>

void Dungeon::updateSunBeams() {
	const int now = GameClock::now();
	sun.Expire(now);
	sun.Trace(map);
	for (const BeamStep& s : sun.Beams()) {
		Fixture& disc = map[s.cell].fixture;
		if (disc.type != SunDisc || !isLockColour(disc.attr) || sunDiscLit(disc))
			continue;
		lightSunDisc(disc);
		sun.LightDisc(s.cell, now);
		sim.events->Play(WorldSound::SunDisc);
		openGates(disc.attr);
		sim.events->Status("The Aten blazes. Somewhere a %s gate grinds open", lockColour(disc.attr).gem);
		sim.events->Note(FieldNote::SunMirrors);
	}
	for (Monster& mon : monsters) {
		const auto col = static_cast<int>(std::floor(mon.CentreX()));
		if (mon.Active() && mon.Alive() && !mon.lurking() && mon.Type()->sunBurns && Sunlit(col, mon.Row()))
			mon.Kindle(SUN_BURN_FIRE);
	}
}
bool Dungeon::TurnMirror() {
	const int cell = MapIndex(static_cast<int>(mapX), static_cast<int>(mapY));
	if (!IsInBounds(static_cast<int>(mapX), static_cast<int>(mapY)) || map[cell].type != Mirror)
		return false;
	const float from = sun.Angle(map, cell);
	turnMirror(map[cell]);
	sun.Trace(map); // the face's new angle follows the beam it now sends
	sun.Swing(cell, from);
	sim.events->Play(WorldSound::MirrorTurn);
	sim.events->Note(FieldNote::SunMirrors);
	return true;
}
