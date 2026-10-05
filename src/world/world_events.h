#ifndef WORLD_EVENTS_H
#define WORLD_EVENTS_H

#include "journal.h"
#include <string>
#include <vector>

// The world's sounds; the app maps them to its sound bank.
enum class WorldSound : unsigned char {
	ArrowHit,  // an arrow in a monster
	ArrowWall, // an arrow in a wall or the floor
	KeyPickup,
	GateOpen,
	GateLocked,
	Lever,
	RockRumble,
	RockCrash,
	Teleport,
	SummonDig,	// a boss's minion digs out of the floor
	SummonDrop, // a boss's minion drops from the ceiling
	Wade,		// a splashing step in half water
	Splash,		// the player lands in half water
};

// What the world, the monsters and the player tell the app: it drains the list at fixed points (after the input
// actions, at the end of the tick) and plays the sound, shows the status line, writes the field note or opens the
// riddle. The world does not reach into the sound bank or the screens.
struct WorldEvent {
	enum class Kind : unsigned char { Sound, Status, Note, AskRiddle };
	Kind kind;
	WorldSound sound = WorldSound::ArrowHit;
	FieldNote note = FieldNote::Health;
	std::string text; // Status
};

class WorldEvents {
  public:
	void Play(WorldSound sound) { list.push_back({WorldEvent::Kind::Sound, sound, {}, {}}); }
	[[gnu::format(printf, 2, 3)]] void Status(const char* format, ...);
	void Note(FieldNote note) { list.push_back({WorldEvent::Kind::Note, {}, note, {}}); }
	void AskRiddle() { list.push_back({WorldEvent::Kind::AskRiddle, {}, {}, {}}); }
	// The events since the last call, oldest first; the list is empty after.
	[[nodiscard]] std::vector<WorldEvent> Take();

  private:
	std::vector<WorldEvent> list;
};

#endif
