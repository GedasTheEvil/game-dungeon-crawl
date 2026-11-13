#ifndef MAP_MARKS_H
#define MAP_MARKS_H

#include "decor.h"
#include "level.h"
#include "monster_kinds.h"
#include "shrines.h"
#include "sun_beam.h"
#include <cstdint>

// The draft map's marks (plans map-symbols-and-zoom, map-legend-sprites): what a cell's symbol shows, one legend row
// and one sprite each. No drawing here: the map (src/ui/map_view.cpp) draws a mark's sprite from the sheet
// media/textures/ui/map_marks.png (tools/textures/map_marks.py). A new mark: a sprite cell in the sheet, a row in
// MARK_DEFS, a case in thingMark or propMark (docs/ui.md, Draft map).

// In the sheet's cell order: a mark's sprite is the cell of its number. Append before None, or move the sprites too.
enum class MapMark : std::uint8_t {
	Entrance,
	Exit,
	SphinxGate,
	SphinxGateAnswered,
	Teleporter,
	RingPlatform,
	NameGate,
	Ladder,
	Treasure,
	TreasureTaken,
	Key,
	Gate,
	BossGate,
	Lever,
	Spikes,
	DeathTrap,
	CeilingSpikes,
	CeilingDeath,
	RockFall,
	DartPlate,
	WallSpikes,
	FireVent,
	PoisonDrip,
	SandGust,
	LightShaft,
	Mirror,
	SunDisc,
	Inscription,
	Page,
	Ankh,
	ShrineIsis, // the shrines in God order (shrines.h)
	ShrineMontu,
	ShrinePtah,
	ShrineAmun,
	ShrineSerket,
	Stargate,
	Dial,
	Brazier,
	PressurePlate, // plan pressure-plate
	Stone,
	None,
};
constexpr int MARK_COUNT = static_cast<int>(MapMark::None);

// The sheet: a grid of square cells, the first row at the PNG's top. The marks' cells come first, in MapMark order,
// then the second looks (MarkDef::stateCell).
constexpr int MAP_MARK_COLUMNS = 8;
constexpr int MAP_MARK_ROWS = 6;
constexpr int MAP_MARK_CELLS = MAP_MARK_COLUMNS * MAP_MARK_ROWS;
// A sprite holds the map cell in its middle, with this much of its own width free on every side (room for the
// pencil's wobble and against the neighbours bleeding in when drawn small): the map draws the sprite that much larger.
constexpr float MAP_MARK_MARGIN = 0.125f;
constexpr int NO_STATE_CELL = -1;

enum class MarkTint : std::uint8_t {
	None, // the colours are in the sprite
	Lock, // a white sprite, tinted with its lock colour's pencil (graphite without one)
};
enum class MarkTurn : std::uint8_t {
	None,
	Shaft, // the sprite shows a light shaft's beam going down: turned to the shaft's way
	Mirror, // ... a mirror pointing down: turned to the mirror's way
	Gust, // ... the wind blowing right: turned round for one blowing left
};
struct MarkDef {
	const char* name; // the legend row's
	MarkTint tint = MarkTint::None;
	MarkTurn turn = MarkTurn::None;
	int stateCell = NO_STATE_CELL; // the sprite of its second look (markSecondLook), if it has one
};
constexpr MarkDef MARK_DEFS[MARK_COUNT] = {
	{"Entrance"},
	{"Exit"},
	{"Sphinx gate"},
	{"Sphinx gate, answered"},
	{"Teleporter"},
	{"Ring platform"},
	{"Name gate"},
	{"Ladder"},
	{"Treasure"},
	{"Treasure, taken"},
	{"Key", MarkTint::Lock},
	{"Gate", MarkTint::Lock, MarkTurn::None, 40}, // open
	{"Boss gate", MarkTint::None, MarkTurn::None, 41}, // open, the seal broken
	{"Lever", MarkTint::Lock, MarkTurn::None, 42}, // pulled
	{"Spikes"},
	{"Death trap"},
	{"Ceiling spikes"},
	{"Ceiling death trap"},
	{"Rock fall", MarkTint::None, MarkTurn::None, 43}, // fallen
	{"Dart plate"},
	{"Wall spikes"},
	{"Fire vent"},
	{"Poison drip"},
	{"Sandstorm", MarkTint::None, MarkTurn::Gust},
	{"Light shaft", MarkTint::None, MarkTurn::Shaft},
	{"Mirror", MarkTint::None, MarkTurn::Mirror},
	{"Sun disc", MarkTint::Lock},
	{"Inscription"},
	{"Notebook page"},
	{"Ankh"},
	{"Shrine of Isis"},
	{"Shrine of Montu"},
	{"Shrine of Ptah"},
	{"Shrine of Amun"},
	{"Shrine of Serket"},
	{"Stargate"},
	{"Dial device"},
	{"Brazier"},
	{"Pressure plate", MarkTint::Lock},
	{"Stone block"},
};
[[nodiscard]] constexpr const MarkDef& markDef(MapMark m) { return MARK_DEFS[static_cast<int>(m)]; }

[[nodiscard]] inline MapMark doorMark(const Tile& t) {
	switch (t.attr) {
	case GateEntrance:
		return MapMark::Entrance;
	case GateExit:
		return MapMark::Exit;
	case GateRiddle:
		return MapMark::SphinxGate;
	case GateTeleport:
		return isNameGate(t) ? MapMark::NameGate : isRingPlatform(t) ? MapMark::RingPlatform : MapMark::Teleporter;
	default: // GateEmpty: a riddle gate once answered (only the game makes one)
		return MapMark::SphinxGateAnswered;
	}
}

// The mark of an object, or of a fixture as a tile (fixtureAsTile). None: no object, a monster other than a mimic, a
// page taken.
[[nodiscard]] inline MapMark thingMark(const Tile& t) {
	switch (t.type) {
	case Door:
		return doorMark(t);
	case MonsterSpawn: // only a mimic gets a symbol: the chest it pretends to be
		if (const MonsterKind* kind = monsterKind(t.attr); kind == nullptr || !ambushes(kind->locomotion))
			return MapMark::None;
		return MapMark::Treasure;
	case Treasure:
	case TrappedTreasure: // the map does not tell them apart
		return MapMark::Treasure;
	case Ladder:
		return MapMark::Ladder;
	case Key:
		return MapMark::Key;
	case Gate:
		return t.attr == BOSS_LOCK ? MapMark::BossGate : MapMark::Gate;
	case Lever:
		return MapMark::Lever;
	case Spike:
		return MapMark::Spikes;
	case Death:
		return MapMark::DeathTrap;
	case CeilingSpikes:
		return MapMark::CeilingSpikes;
	case CeilingDeath:
		return MapMark::CeilingDeath;
	case RockFall:
		return MapMark::RockFall;
	case DartPlate:
		return MapMark::DartPlate;
	case WallSpikes:
		return MapMark::WallSpikes;
	case FireVent:
		return MapMark::FireVent;
	case PoisonDrip:
		return MapMark::PoisonDrip;
	case SandGust:
		return MapMark::SandGust;
	case LightShaft:
		return MapMark::LightShaft;
	case Mirror:
		return MapMark::Mirror;
	case SunDisc:
		return MapMark::SunDisc;
	case Inscription:
		return MapMark::Inscription;
	case Page: // a page of the earlier expedition not taken yet (plan expedition-notes)
		return t.value == 0 ? MapMark::Page : MapMark::None;
	case Ankh:
		return MapMark::Ankh;
	case PressurePlate:
		return MapMark::PressurePlate;
	case Stone:
		return MapMark::Stone;
	default:
		return MapMark::None;
	}
}

// The mark of a prop (the runtime decor type, Dungeon::PropAt): the ones the player can use. None for plain decor.
[[nodiscard]] constexpr MapMark propMark(int decor) {
	if (const std::optional<God> god = shrineGod(decor))
		return static_cast<MapMark>(static_cast<int>(MapMark::ShrineIsis) + static_cast<int>(*god));
	switch (decor) {
	case DECOR_STARGATE:
		return MapMark::Stargate;
	case DECOR_DIAL:
		return MapMark::Dial;
	case DECOR_BRAZIER:
		return MapMark::Brazier;
	default:
		return MapMark::None;
	}
}

// A cell's one symbol: the mark, and the object or fixture it is drawn for (an empty tile for a prop's).
struct ShownMark {
	MapMark mark = MapMark::None;
	Tile thing;
};

// What a cell shows: its object, else its fixture, else the open container of a treasure taken (`emptied`,
// Dungeon::ContainerEmptied), else its prop.
[[nodiscard]] inline ShownMark cellMark(const Tile& cell, int prop, bool emptied) {
	if (const MapMark m = thingMark(cell); m != MapMark::None)
		return {m, cell};
	const Tile fixture = fixtureAsTile(cell.fixture);
	if (const MapMark m = thingMark(fixture); m != MapMark::None)
		return {m, fixture};
	return {emptied ? MapMark::TreasureTaken : propMark(prop), Tile{}};
}

// A mark with two looks shows its second one: an open gate, a pulled lever, a fallen rock.
[[nodiscard]] inline bool markSecondLook(const ShownMark& s) {
	switch (s.mark) {
	case MapMark::Gate:
	case MapMark::BossGate:
		return gateState(s.thing) == GateState::Open;
	case MapMark::Lever:
		return leverPulled(Fixture{s.thing.type, s.thing.attr, s.thing.value});
	case MapMark::RockFall:
		return rockState(s.thing) == RockState::Fallen;
	default:
		return false;
	}
}

// How the map draws a mark: the sheet's cell, the quarter turns clockwise, the lock colour that tints it.
struct MarkSprite {
	int cell = 0;
	int turns = 0;
	int lock = 0; // 0: not a lock colour's (graphite for a MarkTint::Lock mark, else no tint)
};
[[nodiscard]] inline MarkSprite markSprite(const ShownMark& s) {
	const MarkDef& def = markDef(s.mark);
	MarkSprite sprite;
	sprite.cell = markSecondLook(s) ? def.stateCell : static_cast<int>(s.mark);
	switch (def.turn) {
	case MarkTurn::Shaft:
		sprite.turns = static_cast<int>(shaftDir(s.thing));
		break;
	case MarkTurn::Mirror:
		sprite.turns = static_cast<int>(mirrorDir(s.thing));
		break;
	case MarkTurn::Gust:
		sprite.turns = gustDir(Fixture{s.thing.type, s.thing.attr, s.thing.value}) < 0 ? 2 : 0;
		break;
	case MarkTurn::None:
		break;
	}
	if (def.tint == MarkTint::Lock && isLockColour(s.thing.attr))
		sprite.lock = s.thing.attr;
	else if (def.tint == MarkTint::Lock && s.thing.type == Gate && isPlateGateColour(s.thing.attr))
		sprite.lock = plateLockOf(s.thing.attr); // a silver plate gate: its plates' colour (plan pressure-plate)
	return sprite;
}

#endif
