#include "screen_world.h"
#include "../core/timer.h"
#include "../level/message_log.h"
#include "../sim/dungeon.h"

void ScreenWorld::Status(const std::string& text) const {
	messages.Push(text, StatusTone::Plain, GameClock::now(), dungeon.LevelNumber());
}
