#include "settings.h"
#include "game_state.h"
#include "../core/logger.h"
#include "../test/scenario.h"
#include <fstream>
#include <string>

namespace {
constexpr const char* PATH = "saves/settings.txt";
} // namespace

// One "name value" pair a line; unknown names are skipped, so older and newer files both load.
void Settings::Load(RenderSettings& render) {
	if (Scenario::active())
		return;
	std::ifstream f(PATH);
	std::string name;
	int value = 0;
	while (f >> name >> value)
		if (name == "motion_effects")
			render.MotionEffects = value != 0;
}

void Settings::Save(const RenderSettings& render) {
	if (Scenario::active())
		return;
	std::ofstream f(PATH);
	f << "motion_effects " << (render.MotionEffects ? 1 : 0) << "\n";
	if (!f)
		LOG_WARNINGF("game", "Cannot write %s", PATH);
}
