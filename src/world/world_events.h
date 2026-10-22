#ifndef WORLD_EVENTS_H
#define WORLD_EVENTS_H

#include "journal.h"
#include <string>
#include <vector>

// The world's sounds; the app maps them to its sound bank.
enum class WorldSound : unsigned char {
	ArrowHit, // an arrow in a monster
	ArrowWall, // an arrow in a wall or the floor
	StoneHit, // a sling stone or a throwing stick on a monster
	StoneWall, // off a wall or the floor
	KeyPickup,
	GateOpen,
	GateLocked,
	Lever,
	RockRumble,
	RockCrash,
	SandHiss, // a sand fall starts to trickle from the ceiling crack (plan desert-chapter)
	SandPour, // a sand fall pours down
	SandStep, // a step in sand
	Teleport,
	SummonDig, // a boss's minion digs out of the floor
	SummonDrop, // a boss's minion drops from the ceiling
	Wade, // a splashing step in half water
	Splash, // the player lands in half water
	PlateClick, // a dart trap's plate sinks
	Dart, // a dart leaves the wall
	SpikeScrape, // wall spikes twitch before they come out (plan rhythm-traps)
	SpikeThunk, // wall spikes come out
	FireHiss, // a fire vent glows before its jet (plan fire-jet-trap)
	FireRoar, // a fire vent's jet
	MirrorTurn, // a mirror turned on its stand (plan sunlight-mirrors)
	SunDisc, // a sun disc lit by the beam
	Dodge, // the player sidesteps a blow (Dodge skill)
	// A treasure container opens (plan treasure-containers): a wooden lid creaks up (chest, coffer), reed or linen
	// rustles (basket, the offering table's cover), the canopic jars' stone heads scrape off, the sealed jar smashes.
	LidWood,
	LidReed,
	LidStone,
	JarSmash,
	PagePickup, // a torn notebook page picked up (plan expedition-notes): the journal's paper rustle
	DungThud, // a dung ball breaks on a wall, the floor or the player (plan scarab-dung-ball)
};

// A monster's or the player's own sound (CharacterModel: media/sounds/<model>_{die,att,jump,wake,att_ranged,roar}.wav).
enum class CharacterSound : unsigned char { Die, Attack, Jump, Wake, AttackRanged, Roar };
constexpr int PLAYER_CHARACTER = 0; // WorldEvents::PlayCharacter's who for the player; a monster gives its type

// A safe moment for the autosave (plan autosave): the app writes it, unless the player is in danger then
// (Dungeon::PlayerInDanger; a level start is always safe).
enum class AutosaveMoment : unsigned char { LevelStart, Riddle, Boss };

// What the world, the monsters and the player tell the app: it drains the list at fixed points (after the input
// actions, at the end of the tick) and plays the sound, shows the status line, writes the field note or opens the
// riddle (or a name gate's scroll), or writes the autosave. The world does not reach into the sound bank or the
// screens.
struct WorldEvent {
	enum class Kind : unsigned char { Sound, Status, Note, AskRiddle, AskName, CharacterSound, Autosave };
	Kind kind;
	WorldSound sound = WorldSound::ArrowHit;
	FieldNote note = FieldNote::Health;
	std::string text; // Status
	CharacterSound character = CharacterSound::Die;
	int who = PLAYER_CHARACTER; // CharacterSound: a MonsterTypeId, or the player
	AutosaveMoment moment = AutosaveMoment::LevelStart; // Autosave
};

class WorldEvents {
  public:
	void Play(WorldSound sound) { list.push_back({WorldEvent::Kind::Sound, sound, {}, {}}); }
	void PlayCharacter(int who, CharacterSound sound) {
		list.push_back({WorldEvent::Kind::CharacterSound, {}, {}, {}, sound, who});
	}
	[[gnu::format(printf, 2, 3)]] void Status(const char* format, ...);
	void Note(FieldNote note) { list.push_back({WorldEvent::Kind::Note, {}, note, {}}); }
	void AskRiddle() { list.push_back({WorldEvent::Kind::AskRiddle, {}, {}, {}}); }
	// A name gate (plan secret-name-gates): the scroll asks for the level's name.
	void AskName() { list.push_back({WorldEvent::Kind::AskName, {}, {}, {}}); }
	void Autosave(AutosaveMoment moment) {
		list.push_back({WorldEvent::Kind::Autosave, {}, {}, {}, {}, PLAYER_CHARACTER, moment});
	}
	// The events since the last call, oldest first; the list is empty after.
	[[nodiscard]] std::vector<WorldEvent> Take();

  private:
	std::vector<WorldEvent> list;
};

#endif
