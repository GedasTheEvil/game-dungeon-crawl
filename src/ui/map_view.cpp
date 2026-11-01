#include "map_view.h"
#include "../world/sun_beam.h"
#include "ui_draw.h"
#include "../graphics/gl_includes.h"
#include "../state/game_state.h"
#include "../input/bindings.h"
#include "../input/input.h"
#include "../graphics/font.h"
#include <GL/gl.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

using namespace ui;

namespace {
constexpr float PAPER_H = 96.f;
constexpr float PAPER_PAD_X = 6.f;
constexpr float PAPER_PAD_TOP = 11.f; // room for the title
constexpr float PAPER_PAD_BOTTOM = 7.f;
constexpr float MAX_CELL = 5.f; // zoom limit for a small explored part, canvas units per cell
constexpr float JITTER = 0.07f; // pencil wobble, in cells
constexpr float REFERENCE_RES_Y = 720.f; // line widths are in pixels at this height and this cell size
constexpr float REFERENCE_CELL = 2.f;

constexpr Color GRAPHITE = PENCIL;
constexpr Color PENCIL_RED = {0.72f, 0.14f, 0.08f};
constexpr Color LOCK_PENCILS[LOCK_COLOUR_COUNT] = {
	{0.75f, 0.16f, 0.10f}, // red
	{0.12f, 0.27f, 0.62f}, // blue
	{0.14f, 0.48f, 0.18f}, // green
	{0.80f, 0.58f, 0.08f}, // gold
};
constexpr Color ANKH_GOLD = {0.78f, 0.56f, 0.10f};
constexpr Color WATER_BLUE = {0.16f, 0.38f, 0.70f};
constexpr Color PLASMA = {0.10f, 0.50f, 0.55f}; // a teleporter's and a ring platform's
constexpr Color PORTAL_BLUE = {0.14f, 0.36f, 0.80f}; // the exit's plasma (the entrance's is red)
constexpr Color OBSIDIAN = {0.08f, 0.06f, 0.08f}; // the boss gate's
constexpr float WAVE_AMPLITUDE = 0.06f; // cells
constexpr int WAVE_SEGMENTS = 8; // per wave, two crests

// Stable per-stroke randomness, so the sketch does not crawl from frame to frame.
float noise(uint32_t seed) {
	seed ^= seed >> 16;
	seed *= 0x7feb352dU;
	seed ^= seed >> 15;
	seed *= 0x846ca68bU;
	seed ^= seed >> 16;
	return static_cast<float>(seed & 0xffffU) / 32767.5f - 1.f; // [-1, 1]
}

struct Stroke {
	float x0, y0, x1, y1;
	Color c;
	float alpha;
};

// Collects pencil strokes in map cell units, drawn in batches of one line width.
class Sketch {
  public:
	Sketch(float originX, float originY, float cell) : ox(originX), oy(originY), s(cell) {}

	// A hand-drawn line: slightly off at both ends, gone over twice with less pressure the second time.
	void pencil(float x0, float y0, float x1, float y1, Color c, float alpha, uint32_t seed) {
		for (uint32_t pass = 0; pass < 2; pass++) {
			uint32_t k = seed * 8 + pass * 4;
			strokes.push_back(
				{ox + (x0 + JITTER * noise(k)) * s,
				 oy + (y0 + JITTER * noise(k + 1)) * s,
				 ox + (x1 + JITTER * noise(k + 2)) * s,
				 oy + (y1 + JITTER * noise(k + 3)) * s,
				 c,
				 pass == 0 ? alpha : alpha * 0.45f}
			);
		}
	}
	// Closed polygon through `n` points on an ellipse round (cx, cy).
	void loop(float cx, float cy, float rx, float ry, int n, Color c, float alpha, uint32_t seed) {
		for (int i = 0; i < n; i++) {
			float a0 = 2.f * static_cast<float>(M_PI) * static_cast<float>(i) / static_cast<float>(n);
			float a1 = 2.f * static_cast<float>(M_PI) * static_cast<float>(i + 1) / static_cast<float>(n);
			pencil(
				cx + rx * std::cos(a0),
				cy + ry * std::sin(a0),
				cx + rx * std::cos(a1),
				cy + ry * std::sin(a1),
				c,
				alpha,
				seed + static_cast<uint32_t>(i)
			);
		}
	}
	// A wavy line ("~~") from x0 to x1 round height y: two crests, the whole wave a little off as drawn by hand.
	void wave(float x0, float x1, float y, Color c, float alpha, uint32_t seed) {
		const float dx = JITTER * noise(seed) * 0.5f;
		const float dy = JITTER * noise(seed + 1) * 0.5f;
		for (uint32_t pass = 0; pass < 2; pass++) {
			const float lift = pass == 0 ? 0.f : 0.012f;
			for (int k = 0; k < WAVE_SEGMENTS; k++) {
				const float t0 = static_cast<float>(k) / WAVE_SEGMENTS;
				const float t1 = static_cast<float>(k + 1) / WAVE_SEGMENTS;
				const auto yAt = [&](float t) {
					return y + dy + lift + WAVE_AMPLITUDE * std::sin(t * 4.f * static_cast<float>(M_PI));
				};
				strokes.push_back(
					{ox + (x0 + dx + (x1 - x0) * t0) * s,
					 oy + yAt(t0) * s,
					 ox + (x0 + dx + (x1 - x0) * t1) * s,
					 oy + yAt(t1) * s,
					 c,
					 pass == 0 ? alpha : alpha * 0.45f}
				);
			}
		}
	}
	void flush(float widthPx) {
		glLineWidth(widthPx);
		glBegin(GL_LINES);
		for (const Stroke& st : strokes) {
			glColor4f(st.c.r, st.c.g, st.c.b, st.alpha);
			glVertex2f(st.x0, st.y0);
			glVertex2f(st.x1, st.y1);
		}
		glEnd();
		glLineWidth(1);
		strokes.clear();
	}

  private:
	float ox, oy, s;
	std::vector<Stroke> strokes;
};

uint32_t cellSeed(int i, int j, int part) {
	return static_cast<uint32_t>((j * LEVEL_WIDTH + i) * 64 + part) * 2654435761U;
}

Color lockPencil(int colour) { return isLockColour(colour) ? LOCK_PENCILS[colour - 1] : GRAPHITE; }

// Diagonal hatching over a wall cell; lines of neighbouring cells meet, so walls read as one shaded mass.
void hatch(Sketch& sk, int i, int j) {
	auto x = static_cast<float>(i);
	auto y = static_cast<float>(j);
	constexpr float STEPS[] = {0.5f, 1.f, 1.5f, 2.f};
	for (int k = 0; k < 4; k++) {
		float t = STEPS[k];
		if (t <= 1.f)
			sk.pencil(x + t, y, x, y + t, GRAPHITE, 0.30f, cellSeed(i, j, k));
		else
			sk.pencil(x + 1, y + t - 1, x + t - 1, y + 1, GRAPHITE, 0.30f, cellSeed(i, j, k));
	}
}

// Water: blue waves in the lower part of a half-water cell (its water line and below), three rows over a deep one.
void water(Sketch& sk, int i, int j, Tile t) {
	auto x = static_cast<float>(i);
	auto y = static_cast<float>(j);
	if (inHalfWater(t)) {
		sk.wave(x + 0.1f, x + 0.9f, y + 0.4f, WATER_BLUE, 0.75f, cellSeed(i, j, 20));
		sk.wave(x + 0.25f, x + 0.75f, y + 0.2f, WATER_BLUE, 0.5f, cellSeed(i, j, 21));
	} else if (t.structure == Structure::DeepWater) {
		for (int k = 0; k < 3; k++)
			sk.wave(
				x + 0.1f,
				x + 0.9f,
				y + 0.22f + 0.28f * static_cast<float>(k),
				WATER_BLUE,
				0.8f,
				cellSeed(i, j, 22 + k)
			);
	}
}

// Deep water blocks like a wall: a blue line where an explored open cell meets it.
void waterEdge(Sketch& sk, const Dungeon& d, int i, int j) {
	auto x = static_cast<float>(i);
	auto y = static_cast<float>(j);
	auto deep = [&](int ci, int cj) { return d.Explored(ci, cj) && d.Cell(ci, cj).structure == Structure::DeepWater; };
	if (deep(i - 1, j))
		sk.pencil(x, y, x, y + 1, WATER_BLUE, 0.8f, cellSeed(i, j, 26));
	if (deep(i + 1, j))
		sk.pencil(x + 1, y, x + 1, y + 1, WATER_BLUE, 0.8f, cellSeed(i + 1, j, 26));
	if (deep(i, j - 1))
		sk.pencil(x, y, x + 1, y, WATER_BLUE, 0.8f, cellSeed(i, j, 27));
	if (deep(i, j + 1))
		sk.pencil(x, y + 1, x + 1, y + 1, WATER_BLUE, 0.8f, cellSeed(i, j + 1, 27));
}

// Outline between an explored open cell and the explored walls around it.
void outline(Sketch& sk, const Dungeon& d, int i, int j) {
	auto x = static_cast<float>(i);
	auto y = static_cast<float>(j);
	auto wall = [&](int ci, int cj) { return d.Explored(ci, cj) && isWall(d.Cell(ci, cj)); };
	// Seeds come from the edge, not the cell, so a shared edge looks the same whichever side draws it.
	if (wall(i - 1, j))
		sk.pencil(x, y, x, y + 1, GRAPHITE, 0.85f, cellSeed(i, j, 10));
	if (wall(i + 1, j))
		sk.pencil(x + 1, y, x + 1, y + 1, GRAPHITE, 0.85f, cellSeed(i + 1, j, 10));
	if (wall(i, j - 1))
		sk.pencil(x, y, x + 1, y, GRAPHITE, 0.85f, cellSeed(i, j, 11));
	if (wall(i, j + 1))
		sk.pencil(x, y + 1, x + 1, y + 1, GRAPHITE, 0.85f, cellSeed(i, j + 1, 11));
}

void arch(Sketch& sk, float x, float y, Color c, uint32_t seed) {
	sk.pencil(x + 0.22f, y, x + 0.22f, y + 0.55f, c, 0.9f, seed);
	sk.pencil(x + 0.78f, y, x + 0.78f, y + 0.55f, c, 0.9f, seed + 1);
	constexpr int SEGMENTS = 4;
	for (int k = 0; k < SEGMENTS; k++) {
		float a0 = static_cast<float>(M_PI) * static_cast<float>(k) / SEGMENTS;
		float a1 = static_cast<float>(M_PI) * static_cast<float>(k + 1) / SEGMENTS;
		sk.pencil(
			x + 0.5f + 0.28f * std::cos(a0),
			y + 0.55f + 0.3f * std::sin(a0),
			x + 0.5f + 0.28f * std::cos(a1),
			y + 0.55f + 0.3f * std::sin(a1),
			c,
			0.9f,
			seed + 2 + static_cast<uint32_t>(k)
		);
	}
}

void zigzag(Sketch& sk, float x, float y, float height, Color c, uint32_t seed) {
	constexpr int TEETH = 3;
	for (int k = 0; k < TEETH; k++) {
		float left = x + 0.1f + 0.8f * static_cast<float>(k) / TEETH;
		float right = x + 0.1f + 0.8f * static_cast<float>(k + 1) / TEETH;
		float tip = (left + right) / 2;
		sk.pencil(left, y + 0.05f, tip, y + height, c, 0.9f, seed + static_cast<uint32_t>(2 * k));
		sk.pencil(tip, y + height, right, y + 0.05f, c, 0.9f, seed + static_cast<uint32_t>(2 * k + 1));
	}
}

// The teleporter's two columns, a capital on each.
void columns(Sketch& sk, float x, float y, uint32_t seed) {
	sk.pencil(x + 0.15f, y, x + 0.15f, y + 0.9f, GRAPHITE, 0.9f, seed);
	sk.pencil(x + 0.85f, y, x + 0.85f, y + 0.9f, GRAPHITE, 0.9f, seed + 1);
	sk.pencil(x + 0.07f, y + 0.9f, x + 0.23f, y + 0.9f, GRAPHITE, 0.9f, seed + 2);
	sk.pencil(x + 0.77f, y + 0.9f, x + 0.93f, y + 0.9f, GRAPHITE, 0.9f, seed + 3);
}

// What a cell's symbol shows, one legend row each (MARK_NAMES).
enum class MapMark : std::uint8_t {
	Entrance,
	Exit,
	SphinxGate,
	Doorway,
	Teleporter,
	RingPlatform,
	NameGate,
	Ladder,
	Treasure,
	Key,
	Gate,
	BossGate,
	Lever,
	Spikes,
	DeathTrap,
	RockFall,
	DartPlate,
	WallSpikes,
	FireVent,
	PoisonDrip,
	LightShaft,
	Mirror,
	SunDisc,
	Inscription,
	Page,
	Ankh,
	None,
};
constexpr int MARK_COUNT = static_cast<int>(MapMark::None);
constexpr const char* MARK_NAMES[MARK_COUNT] = {
	"Entrance",	  "Exit",	   "Sphinx gate", "Sphinx",		   "Teleporter", "Ring platform", "Name gate",
	"Ladder",	  "Treasure",  "Key",		  "Gate",		   "Boss gate",	 "Lever",		  "Spikes",
	"Death trap", "Rock fall", "Dart plate",  "Wall spikes",   "Fire vent",	 "Poison drip",	  "Light shaft",
	"Mirror",	  "Sun disc",  "Inscription", "Notebook page", "Ankh",
};

MapMark doorMark(const Tile& t) {
	switch (t.attr) {
	case GateEntrance:
		return MapMark::Entrance;
	case GateExit:
		return MapMark::Exit;
	case GateRiddle:
		return MapMark::SphinxGate;
	case GateTeleport:
		return isNameGate(t) ? MapMark::NameGate : isRingPlatform(t) ? MapMark::RingPlatform : MapMark::Teleporter;
	default:
		return MapMark::Doorway;
	}
}

// None: no object, a monster other than a mimic, a page taken.
MapMark markOf(const Tile& t) {
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
	default:
		return MapMark::None;
	}
}

// The tile a cell's symbol is drawn for: the object, else the fixture.
Tile shownTile(const Tile& t) { return markOf(t) != MapMark::None ? t : fixtureAsTile(t.fixture); }

void gate(Sketch& sk, float x, float y, Color c, bool open, uint32_t seed) {
	float bottom = open ? 0.75f : 0.f; // an open gate hangs up in the ceiling
	sk.pencil(x + 0.15f, y + 0.95f, x + 0.85f, y + 0.95f, c, 0.95f, seed);
	if (!open)
		sk.pencil(x + 0.15f, y + 0.05f, x + 0.85f, y + 0.05f, c, 0.95f, seed + 1);
	for (int k = 0; k < 3; k++) {
		float u = x + 0.3f + 0.2f * static_cast<float>(k);
		sk.pencil(u, y + bottom, u, y + 0.95f, c, 0.95f, seed + 2 + static_cast<uint32_t>(k));
	}
}

// Draws t's symbol; false if t has none (markOf).
bool symbol(Sketch& sk, int i, int j, Tile t) {
	auto x = static_cast<float>(i);
	auto y = static_cast<float>(j);
	uint32_t seed = cellSeed(i, j, 20);
	switch (markOf(t)) {
	case MapMark::Entrance: // the sphinx's arch, the portal in it in its plasma's colour: red in, blue out
	case MapMark::Exit: {
		const Color c = t.attr == GateExit ? PORTAL_BLUE : PENCIL_RED;
		arch(sk, x, y, GRAPHITE, seed);
		sk.loop(x + 0.5f, y + 0.42f, 0.17f, 0.3f, 8, c, 0.95f, seed + 8);
		sk.loop(x + 0.5f, y + 0.42f, 0.07f, 0.13f, 6, c, 0.85f, seed + 16);
		break;
	}
	case MapMark::SphinxGate: { // a door, a question mark in it
		const Color c = LOCK_PENCILS[1];
		sk.pencil(x + 0.18f, y, x + 0.18f, y + 0.92f, GRAPHITE, 0.9f, seed);
		sk.pencil(x + 0.82f, y, x + 0.82f, y + 0.92f, GRAPHITE, 0.9f, seed + 1);
		sk.pencil(x + 0.12f, y + 0.92f, x + 0.88f, y + 0.92f, GRAPHITE, 0.9f, seed + 2);
		sk.pencil(x + 0.34f, y + 0.6f, x + 0.42f, y + 0.74f, c, 0.95f, seed + 8);
		sk.pencil(x + 0.42f, y + 0.74f, x + 0.58f, y + 0.76f, c, 0.95f, seed + 9);
		sk.pencil(x + 0.58f, y + 0.76f, x + 0.66f, y + 0.62f, c, 0.95f, seed + 10);
		sk.pencil(x + 0.66f, y + 0.62f, x + 0.5f, y + 0.47f, c, 0.95f, seed + 11);
		sk.pencil(x + 0.5f, y + 0.47f, x + 0.5f, y + 0.32f, c, 0.95f, seed + 12);
		sk.loop(x + 0.5f, y + 0.16f, 0.04f, 0.04f, 5, c, 0.95f, seed + 13);
		break;
	}
	case MapMark::Doorway:
		arch(sk, x, y, GRAPHITE, seed);
		break;
	case MapMark::Teleporter: // the columns, the plasma between them
		columns(sk, x, y, seed);
		sk.loop(x + 0.5f, y + 0.45f, 0.2f, 0.32f, 8, PLASMA, 0.9f, seed + 4);
		sk.loop(x + 0.5f, y + 0.45f, 0.08f, 0.14f, 6, PLASMA, 0.8f, seed + 12);
		break;
	case MapMark::RingPlatform: // the platform, three rings stacked over it
		sk.pencil(x + 0.1f, y + 0.06f, x + 0.9f, y + 0.06f, GRAPHITE, 0.95f, seed);
		for (int k = 0; k < 3; k++)
			sk.loop(
				x + 0.5f,
				y + 0.3f + 0.22f * static_cast<float>(k),
				0.32f,
				0.07f,
				8,
				PLASMA,
				0.9f,
				seed + 1 + 8 * static_cast<uint32_t>(k)
			);
		break;
	case MapMark::NameGate: // the columns, sealed by a cartouche: the level's name opens it
		columns(sk, x, y, seed);
		sk.loop(x + 0.5f, y + 0.5f, 0.13f, 0.26f, 7, ANKH_GOLD, 0.95f, seed + 4);
		sk.pencil(x + 0.33f, y + 0.2f, x + 0.67f, y + 0.2f, ANKH_GOLD, 0.95f, seed + 11);
		break;
	case MapMark::Ladder:
		sk.pencil(x + 0.3f, y, x + 0.3f, y + 1, GRAPHITE, 0.8f, seed);
		sk.pencil(x + 0.7f, y, x + 0.7f, y + 1, GRAPHITE, 0.8f, seed + 1);
		for (int k = 0; k < 3; k++) {
			float v = y + 0.2f + 0.3f * static_cast<float>(k);
			sk.pencil(x + 0.3f, v, x + 0.7f, v, GRAPHITE, 0.8f, seed + 2 + static_cast<uint32_t>(k));
		}
		break;
	case MapMark::Treasure: // X marks the spot
		sk.pencil(x + 0.25f, y + 0.2f, x + 0.75f, y + 0.7f, PENCIL_RED, 0.95f, seed);
		sk.pencil(x + 0.25f, y + 0.7f, x + 0.75f, y + 0.2f, PENCIL_RED, 0.95f, seed + 1);
		break;
	case MapMark::Spikes:
		zigzag(sk, x, y, 0.4f, GRAPHITE, seed);
		break;
	case MapMark::DeathTrap:
		zigzag(sk, x, y, 0.65f, PENCIL_RED, seed);
		break;
	case MapMark::Key: {
		Color c = lockPencil(t.attr);
		sk.loop(x + 0.3f, y + 0.5f, 0.14f, 0.14f, 6, c, 0.95f, seed);
		sk.pencil(x + 0.44f, y + 0.5f, x + 0.85f, y + 0.5f, c, 0.95f, seed + 6);
		sk.pencil(x + 0.72f, y + 0.5f, x + 0.72f, y + 0.35f, c, 0.95f, seed + 7);
		sk.pencil(x + 0.84f, y + 0.5f, x + 0.84f, y + 0.35f, c, 0.95f, seed + 8);
		break;
	}
	case MapMark::Gate:
		gate(sk, x, y, lockPencil(t.attr), gateState(t) == GateState::Open, seed);
		break;
	case MapMark::BossGate: { // the gate in obsidian, a seal on it while closed: the boss's death breaks it
		const bool open = gateState(t) == GateState::Open;
		gate(sk, x, y, OBSIDIAN, open, seed);
		if (!open)
			sk.loop(x + 0.5f, y + 0.5f, 0.17f, 0.17f, 4, PENCIL_RED, 0.95f, seed + 5);
		break;
	}
	case MapMark::Lever: {
		Color c = lockPencil(t.attr);
		float tipX = leverPulled(Fixture{t.type, t.attr, t.value}) ? 0.25f : 0.75f;
		sk.pencil(x + 0.25f, y + 0.08f, x + 0.75f, y + 0.08f, c, 0.95f, seed);
		sk.pencil(x + 0.5f, y + 0.08f, x + tipX, y + 0.55f, c, 0.95f, seed + 1);
		sk.loop(x + tipX, y + 0.6f, 0.07f, 0.07f, 5, c, 0.95f, seed + 2);
		break;
	}
	case MapMark::RockFall:
		if (rockState(t) == RockState::Fallen) { // a boulder on the floor
			sk.loop(x + 0.5f, y + 0.25f, 0.28f, 0.2f, 7, GRAPHITE, 0.9f, seed);
		} else { // loose ceiling: a crack
			sk.pencil(x + 0.15f, y + 0.95f, x + 0.35f, y + 0.78f, GRAPHITE, 0.9f, seed);
			sk.pencil(x + 0.35f, y + 0.78f, x + 0.55f, y + 0.9f, GRAPHITE, 0.9f, seed + 1);
			sk.pencil(x + 0.55f, y + 0.9f, x + 0.8f, y + 0.72f, GRAPHITE, 0.9f, seed + 2);
		}
		break;
	case MapMark::DartPlate: // a slab in the floor, the three dart holes in the wall above it
		sk.pencil(x + 0.2f, y + 0.08f, x + 0.8f, y + 0.08f, GRAPHITE, 0.95f, seed);
		sk.pencil(x + 0.2f, y + 0.16f, x + 0.8f, y + 0.16f, GRAPHITE, 0.95f, seed + 1);
		for (int k = 0; k < 3; k++)
			sk.loop(
				x + 0.28f + 0.22f * static_cast<float>(k),
				y + 0.4f,
				0.06f,
				0.06f,
				5,
				GRAPHITE,
				0.9f,
				seed + 2 + static_cast<uint32_t>(k)
			);
		break;
	case MapMark::WallSpikes: // the slots in the back wall the spikes come out of, a 3 x 3 grid
		for (int c = 0; c < 3; c++)
			for (int r = 0; r < 3; r++)
				sk.loop(
					x + 0.25f + 0.25f * static_cast<float>(c),
					y + 0.22f + 0.25f * static_cast<float>(r),
					0.05f,
					0.05f,
					5,
					GRAPHITE,
					0.9f,
					seed + static_cast<uint32_t>(c * 3 + r)
				);
		break;
	case MapMark::FireVent: // the vent's mouth low in the back wall, a tongue of flame over it
		sk.pencil(x + 0.3f, y + 0.12f, x + 0.7f, y + 0.12f, GRAPHITE, 0.95f, seed);
		sk.pencil(x + 0.3f, y + 0.22f, x + 0.7f, y + 0.22f, GRAPHITE, 0.95f, seed + 1);
		sk.pencil(x + 0.5f, y + 0.28f, x + 0.42f, y + 0.55f, GRAPHITE, 0.9f, seed + 2);
		sk.pencil(x + 0.42f, y + 0.55f, x + 0.55f, y + 0.8f, GRAPHITE, 0.9f, seed + 3);
		break;
	case MapMark::PoisonDrip: // the puddle on the floor, the drop hanging from the ceiling over it, its fall
		sk.loop(x + 0.5f, y + 0.1f, 0.32f, 0.06f, 7, GRAPHITE, 0.95f, seed);
		sk.loop(x + 0.5f, y + 0.84f, 0.07f, 0.09f, 5, GRAPHITE, 0.9f, seed + 1);
		sk.pencil(x + 0.5f, y + 0.66f, x + 0.5f, y + 0.5f, GRAPHITE, 0.8f, seed + 2);
		sk.pencil(x + 0.5f, y + 0.4f, x + 0.5f, y + 0.24f, GRAPHITE, 0.8f, seed + 3);
		break;
	case MapMark::LightShaft: { // rays fanning out of the crack the way the beam goes
		const BeamDir d = shaftDir(t);
		const float dx = static_cast<float>(beamDx(d)), dy = static_cast<float>(beamDy(d));
		const float sx = x + 0.5f - 0.4f * dx, sy = y + 0.5f - 0.4f * dy;
		for (int k = -1; k <= 1; k++) {
			const float spread = 0.18f * static_cast<float>(k);
			sk.pencil(
				sx,
				sy,
				sx + 0.7f * dx - spread * dy,
				sy + 0.7f * dy + spread * dx,
				ANKH_GOLD,
				0.9f,
				seed + static_cast<uint32_t>(k + 1)
			);
		}
		break;
	}
	case MapMark::Mirror: { // the disc on its post, a stroke the way it points
		const BeamDir d = mirrorDir(t);
		sk.pencil(x + 0.5f, y + 0.05f, x + 0.5f, y + 0.4f, GRAPHITE, 0.9f, seed);
		sk.loop(x + 0.5f, y + 0.55f, 0.15f, 0.15f, 7, GRAPHITE, 0.9f, seed + 1);
		sk.pencil(
			x + 0.5f,
			y + 0.55f,
			x + 0.5f + 0.38f * static_cast<float>(beamDx(d)),
			y + 0.55f + 0.38f * static_cast<float>(beamDy(d)),
			ANKH_GOLD,
			0.95f,
			seed + 2
		);
		break;
	}
	case MapMark::SunDisc: { // a disc with rays, in its lock colour
		const Color c = lockPencil(t.attr);
		sk.loop(x + 0.5f, y + 0.55f, 0.16f, 0.16f, 7, c, 0.95f, seed);
		for (int k = 0; k < 6; k++) {
			const float a = 6.2832f * static_cast<float>(k) / 6.f;
			sk.pencil(
				x + 0.5f + 0.22f * std::cos(a),
				y + 0.55f + 0.22f * std::sin(a),
				x + 0.5f + 0.34f * std::cos(a),
				y + 0.55f + 0.34f * std::sin(a),
				c,
				0.9f,
				seed + 1 + static_cast<uint32_t>(k)
			);
		}
		break;
	}
	case MapMark::Inscription: // a cartouche on the back wall: an upright oval on its bar
		sk.loop(x + 0.5f, y + 0.55f, 0.14f, 0.24f, 7, GRAPHITE, 0.9f, seed);
		sk.pencil(x + 0.32f, y + 0.28f, x + 0.68f, y + 0.28f, GRAPHITE, 0.95f, seed + 1);
		break;
	case MapMark::Page: // a page of the earlier expedition not taken yet: a sheet with lines of writing (plan
						// expedition-notes)
		sk.pencil(x + 0.3f, y + 0.12f, x + 0.3f, y + 0.82f, GRAPHITE, 0.9f, seed);
		sk.pencil(x + 0.3f, y + 0.82f, x + 0.72f, y + 0.82f, GRAPHITE, 0.9f, seed + 1);
		sk.pencil(x + 0.72f, y + 0.82f, x + 0.72f, y + 0.12f, GRAPHITE, 0.9f, seed + 2);
		sk.pencil(x + 0.72f, y + 0.12f, x + 0.3f, y + 0.12f, GRAPHITE, 0.9f, seed + 3);
		for (int k = 0; k < 3; k++) {
			const float v = y + 0.32f + 0.17f * static_cast<float>(k);
			sk.pencil(x + 0.38f, v, x + 0.64f, v, INK_FADED, 0.9f, seed + 4 + static_cast<uint32_t>(k));
		}
		break;
	case MapMark::Ankh:
		sk.loop(x + 0.5f, y + 0.74f, 0.12f, 0.16f, 7, ANKH_GOLD, 0.95f, seed);
		sk.pencil(x + 0.28f, y + 0.55f, x + 0.72f, y + 0.55f, ANKH_GOLD, 0.95f, seed + 7);
		sk.pencil(x + 0.5f, y + 0.58f, x + 0.5f, y + 0.08f, ANKH_GOLD, 0.95f, seed + 8);
		break;
	case MapMark::None:
		return false;
	}
	return true;
}

void drawBackground(float canvasW) {
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	Game().assets.textures.loadingBackground.Bind();
	glColor3f(0.30f, 0.24f, 0.18f);
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	glTexCoord2f(1, 0);
	glVertex2f(canvasW, 0);
	glTexCoord2f(1, 1);
	glVertex2f(canvasW, CANVAS_H);
	glTexCoord2f(0, 1);
	glVertex2f(0, CANVAS_H);
	glEnd();
	beginShapes();
	constexpr float VIGNETTE = 20.f;
	ring(Rect{0, 0, canvasW, CANVAS_H}.inset(VIGNETTE), VIGNETTE, BLACK, 0.f, 0.85f);
}

// The papyrus sheet is landscape, the map portrait: the texture is turned a quarter round.
void drawPaper(const Rect& r) {
	beginShapes();
	fillRect({r.x + 1.2f, r.y - 1.2f, r.w, r.h}, BLACK, BLACK, 0.45f); // shadow
	glEnable(GL_TEXTURE_2D);
	Game().assets.textures.papyrus.Bind();
	glColor4f(1, 1, 1, 1);
	glBegin(GL_QUADS);
	glTexCoord2f(1, 0);
	glVertex2f(r.x, r.y);
	glTexCoord2f(1, 1);
	glVertex2f(r.x + r.w, r.y);
	glTexCoord2f(0, 1);
	glVertex2f(r.x + r.w, r.y + r.h);
	glTexCoord2f(0, 0);
	glVertex2f(r.x, r.y + r.h);
	glEnd();
	glDisable(GL_TEXTURE_2D);
}
// ---- legend and zoom (plan map-symbols-and-zoom) ----
constexpr float LEGEND_ROW_H = 4.f; // at the most; a long legend packs its rows closer
constexpr float LEGEND_CELL = 2.8f; // a symbol's cell
constexpr float LEGEND_PAD = 2.5f;
constexpr float LEGEND_TEXT_GAP = 1.4f; // symbol to name
constexpr float LEGEND_HEAD = 8.f; // the heading, from the slip's top to the first row
constexpr float LEGEND_GAP = 2.f; // slip to sheet
constexpr float LEGEND_MARGIN = 2.f; // slip to the window's left edge
constexpr float CLIP_BOTTOM = 5.5f; // the sketch's clip, from the sheet's bottom: over the key hint
constexpr float CLIP_TOP = 9.f; // ... from its top: under the title
constexpr float CLIP_SIDE = 1.5f;
constexpr float PAN_STEP = 10.f; // canvas units an arrow press
constexpr const char* YOU = "You";

struct LegendRow {
	MapMark mark;
	Tile sample; // the first explored tile with that mark: the row draws its symbol
};

// The marks on the explored cells, in MapMark order.
std::vector<LegendRow> legendRows(const Dungeon& d) {
	std::array<bool, MARK_COUNT> seen{};
	std::array<Tile, MARK_COUNT> samples{};
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j)) {
				const Tile t = shownTile(d.Cell(i, j));
				const MapMark m = markOf(t);
				if (m != MapMark::None && !seen[static_cast<size_t>(m)]) {
					seen[static_cast<size_t>(m)] = true;
					samples[static_cast<size_t>(m)] = t;
				}
			}
	std::vector<LegendRow> rows;
	for (size_t k = 0; k < seen.size(); k++)
		if (seen[k])
			rows.push_back({static_cast<MapMark>(k), samples[k]});
	return rows;
}

// One row per mark, then the red ring of "you are here".
void drawLegend(const Rect& slip, const std::vector<LegendRow>& rows, float rowH, float resScale) {
	drawPaper(slip);
	const float cell = std::min(LEGEND_CELL, rowH * 0.85f);
	const float top = slip.y + slip.h - LEGEND_HEAD;
	const auto rowBottom = [&](size_t k) { return top - rowH * static_cast<float>(k + 1); };
	beginShapes();
	glEnable(GL_LINE_SMOOTH);
	const float px = 1.5f * resScale * std::sqrt(cell / REFERENCE_CELL);
	for (size_t k = 0; k <= rows.size(); k++) {
		Sketch sk(slip.x + LEGEND_PAD, rowBottom(k) + (rowH - cell) / 2, cell);
		if (k < rows.size())
			symbol(sk, 0, 0, rows[k].sample);
		else
			sk.loop(0.5f, 0.5f, 0.45f, 0.45f, 9, PENCIL_RED, 1.f, 7u);
		sk.flush(px);
	}
	diamond(slip.x + LEGEND_PAD + cell / 2, rowBottom(rows.size()) + rowH / 2, cell * 0.17f, PENCIL_RED, 1.f);
	glDisable(GL_LINE_SMOOTH);

	beginText();
	Font& font = Game().assets.fonts.hudSmall;
	textCentered(font, slip.cx(), slip.y + slip.h - LEGEND_HEAD + 2.f, "Legend", INK);
	for (size_t k = 0; k <= rows.size(); k++)
		textMiddle(
			font,
			slip.x + LEGEND_PAD + cell + LEGEND_TEXT_GAP,
			rowBottom(k) + rowH / 2,
			k < rows.size() ? MARK_NAMES[static_cast<size_t>(rows[k].mark)] : YOU,
			INK
		);
}

// Clips the sketch to `r` (canvas units, the modelview is the identity), so a zoomed one stays on the sheet.
void clipTo(const Rect& r) {
	const GLdouble planes[4][4] = {
		{1, 0, 0, -r.x},
		{-1, 0, 0, r.x + r.w},
		{0, 1, 0, -r.y},
		{0, -1, 0, r.y + r.h},
	};
	for (int k = 0; k < 4; k++) {
		glClipPlane(static_cast<GLenum>(GL_CLIP_PLANE0 + k), planes[k]);
		glEnable(static_cast<GLenum>(GL_CLIP_PLANE0 + k));
	}
}

// The arrows and the bound move keys: the view goes that way, the sketch the other.
void pan(MapZoom& zoom, const MapFit& fit, NavKey key) {
	switch (key) {
	case NavKey::Left:
		zoom.Pan(fit, PAN_STEP, 0.f);
		break;
	case NavKey::Right:
		zoom.Pan(fit, -PAN_STEP, 0.f);
		break;
	case NavKey::Up:
		zoom.Pan(fit, 0.f, -PAN_STEP);
		break;
	case NavKey::Down:
		zoom.Pan(fit, 0.f, PAN_STEP);
		break;
	case NavKey::Activate:
	case NavKey::None:
		break;
	}
}

void unclip() {
	for (int k = 0; k < 4; k++)
		glDisable(static_cast<GLenum>(GL_CLIP_PLANE0 + k));
}
} // namespace

void DraftMap::Draw() {
	const Dungeon& d = Game().dungeon;
	float resScale = static_cast<float>(Game().render.resY) / REFERENCE_RES_Y;
	if (d.LevelNumber() != level) {
		level = d.LevelNumber();
		zoom.Reset();
		dragging = false;
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	canvasW = beginSquareCanvas(CANVAS_H, Game().render.resX, Game().render.resY);
	glLoadIdentity();
	glDisable(GL_DEPTH_TEST);

	drawBackground(canvasW);

	// The sheet fits the whole level; the sketch is scaled up to fill it with the part explored so far.
	float drawH = PAPER_H - PAPER_PAD_TOP - PAPER_PAD_BOTTOM;
	float drawW = drawH / LEVEL_HEIGHT * LEVEL_WIDTH;
	Rect paper = {(canvasW - drawW) / 2 - PAPER_PAD_X, (CANVAS_H - PAPER_H) / 2, drawW + 2 * PAPER_PAD_X, PAPER_H};
	drawPaper(paper);

	// The legend slip left of the sheet; in a narrow window it lies over the sheet's margin.
	const std::vector<LegendRow> rows = legendRows(d);
	Font& small = Game().assets.fonts.hudSmall;
	float namesW = std::max(small.TextWidth("Legend"), small.TextWidth(YOU));
	for (const LegendRow& row : rows)
		namesW = std::max(namesW, small.TextWidth(MARK_NAMES[static_cast<size_t>(row.mark)]));
	const auto rowCount = static_cast<float>(rows.size() + 1);
	const float rowH = std::min(LEGEND_ROW_H, (PAPER_H - LEGEND_HEAD - LEGEND_PAD) / rowCount);
	const float slipW = 2 * LEGEND_PAD + std::min(LEGEND_CELL, rowH * 0.85f) + LEGEND_TEXT_GAP + namesW;
	const float slipH = LEGEND_HEAD + rowH * rowCount + LEGEND_PAD;
	const float slipX = std::max(LEGEND_MARGIN, paper.x - LEGEND_GAP - slipW);
	const Rect slip = {slipX, paper.y + paper.h - slipH, slipW, slipH};
	Rect area = {paper.x + PAPER_PAD_X, paper.y + PAPER_PAD_BOTTOM, drawW, drawH};

	int minI = LEVEL_WIDTH, maxI = -1, minJ = LEVEL_HEIGHT, maxJ = -1;
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j)) {
				minI = std::min(minI, i);
				maxI = std::max(maxI, i);
				minJ = std::min(minJ, j);
				maxJ = std::max(maxJ, j);
			}
	if (maxI < 0) { // nothing explored: the whole level
		minI = minJ = 0;
		maxI = LEVEL_WIDTH - 1;
		maxJ = LEVEL_HEIGHT - 1;
	}
	auto spanI = static_cast<float>(maxI - minI + 1);
	auto spanJ = static_cast<float>(maxJ - minJ + 1);
	fit = {
		area,
		std::min({area.w / spanI, area.h / spanJ, MAX_CELL}),
		static_cast<float>(minI) + spanI / 2,
		static_cast<float>(minJ) + spanJ / 2
	};
	const MapFrame frame = zoom.Frame(fit);
	const float cell = frame.cell;
	const float originX = frame.originX;
	const float originY = frame.originY;

	float pxScale = resScale * std::sqrt(cell / REFERENCE_CELL);
	beginShapes();
	clipTo({paper.x + CLIP_SIDE, paper.y + CLIP_BOTTOM, paper.w - 2 * CLIP_SIDE, paper.h - CLIP_BOTTOM - CLIP_TOP});
	glEnable(GL_LINE_SMOOTH);
	Sketch sk(originX, originY, cell);
	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j) && isWall(d.Cell(i, j)))
				hatch(sk, i, j);
	sk.flush(1.f * pxScale);

	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j) && !isWall(d.Cell(i, j)))
				outline(sk, d, i, j);
	sk.flush(2.f * pxScale);

	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j)) {
				water(sk, i, j, d.Cell(i, j));
				if (!isWall(d.Cell(i, j)) && d.Cell(i, j).structure != Structure::DeepWater)
					waterEdge(sk, d, i, j);
			}
	sk.flush(1.5f * pxScale);

	for (int j = 0; j < LEVEL_HEIGHT; j++)
		for (int i = 0; i < LEVEL_WIDTH; i++)
			if (d.Explored(i, j)) // one symbol a cell: the object, else the fixture
				symbol(sk, i, j, shownTile(d.Cell(i, j)));
	sk.flush(1.5f * pxScale);

	// "You are here": a red ring round the archaeologist.
	float px, py;
	Game().dungeon.getC(px, py);
	sk.loop(px, py + 0.5f, 0.6f, 0.6f, 9, PENCIL_RED, 1.f, 7u);
	sk.flush(2.f * pxScale);
	diamond(originX + px * cell, originY + (py + 0.5f) * cell, cell * 0.22f, PENCIL_RED, 1.f);
	glDisable(GL_LINE_SMOOTH);
	unclip();
	drawLegend(slip, rows, rowH, resScale); // over the sketch, where it lies over the sheet

	std::string title = "Level " + std::to_string(Game().dungeon.LevelNumber());
	if (!Game().dungeon.LevelTitle().empty())
		title += ": " + Game().dungeon.LevelTitle();
	beginText();
	textCentered(Game().assets.fonts.status, paper.cx(), paper.y + paper.h - PAPER_PAD_TOP + 2.5f, title.c_str(), INK);
	textCentered(
		small,
		paper.cx(),
		paper.y + 2.f,
		"Wheel, + / -: zoom    Drag, arrows: pan    M / Esc: close",
		INK_FADED
	);
	ScreenTabs::Draw();

	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
	glColor3f(1, 1, 1);
}

void DraftMap::toCanvas(int x, int y, float& cx, float& cy) const {
	ui::toCanvas({0, 0, canvasW, CANVAS_H}, Game().render.resX, Game().render.resY, x, y, cx, cy);
}

void DraftMap::KeyPressed(unsigned char key) {
	if (fit.cell <= 0.f) // not drawn yet
		return;
	if (key == '+' || key == '=')
		zoom.ZoomAt(fit, MapZoom::STEP, fit.area.cx(), fit.area.cy());
	else if (key == '-' || key == '_')
		zoom.ZoomAt(fit, 1.f / MapZoom::STEP, fit.area.cx(), fit.area.cy());
	else
		pan(zoom, fit, navKeyOf(InputKey::Char(key), Game().settings.controls));
}

void DraftMap::SpecialKeyPressed(int key) {
	if (fit.cell > 0.f)
		pan(zoom, fit, navKeyOf(InputKey::Special(key), Game().settings.controls));
}

void DraftMap::MouseFunction(int button, int state, int x, int y) {
	if (fit.cell <= 0.f)
		return;
	float cx, cy;
	toCanvas(x, y, cx, cy);
	if (button == MOUSE_WHEEL_UP || button == MOUSE_WHEEL_DOWN) {
		if (state != GLUT_DOWN)
			return;
		if (!fit.area.contains(cx, cy)) { // off the sketch: round its centre
			cx = fit.area.cx();
			cy = fit.area.cy();
		}
		zoom.ZoomAt(fit, button == MOUSE_WHEEL_UP ? MapZoom::STEP : 1.f / MapZoom::STEP, cx, cy);
	} else if (button == MOUSE_LEFT_BUTTON) {
		dragging = state == GLUT_DOWN;
		dragX = cx;
		dragY = cy;
	}
}

void DraftMap::MouseMotion(int x, int y) {
	if (!dragging || fit.cell <= 0.f)
		return;
	float cx, cy;
	toCanvas(x, y, cx, cy);
	zoom.Pan(fit, cx - dragX, cy - dragY);
	dragX = cx;
	dragY = cy;
}
