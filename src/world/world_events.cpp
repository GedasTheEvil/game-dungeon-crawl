#include "world_events.h"
#include <cstdarg>
#include <cstdio>
#include <utility>

void WorldEvents::Status(const char* format, ...) {
	char buf[256];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	list.push_back({WorldEvent::Kind::Status, {}, {}, buf});
}

void WorldEvents::Status(StatusTone tone, const char* format, ...) {
	char buf[256];
	va_list args;
	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);
	WorldEvent event{WorldEvent::Kind::Status, {}, {}, buf};
	event.tone = tone;
	list.push_back(event);
}

std::vector<WorldEvent> WorldEvents::Take() { return std::exchange(list, {}); }
