#ifndef GAMEPLAY_TRAPS_H
#define GAMEPLAY_TRAPS_H

#include "gameplay_shared.h"

// Rock fall: stepping into the cell starts the rumble, the rock drops after ROCK_WARN_MS and lands
// ROCK_FALL_MS later. Walking on without stopping, sprinting, jumping on or stepping back gets the player clear.
constexpr int ROCK_WARN_MS = 900;
constexpr int ROCK_FALL_MS = 300;
constexpr int SAND_POUR_MS = ROCK_FALL_MS + 600; // a sand fall's stream, from the end of its warning
// A pour stays drawn this long past SAND_POUR_MS: the last sand settles.
constexpr int SAND_POUR_TAIL_MS = 400;
// Armor does not help against a boulder: a hit on the head crushes, the edge of it still breaks a leg.
constexpr int ROCK_CRUSH_DAMAGE = 1000;
constexpr int ROCK_GRAZE_DAMAGE = 50;
constexpr float ROCK_CRUSH_HALF_WIDTH = 0.3f; // tiles from the cell centre
constexpr float ROCK_GRAZE_HALF_WIDTH = 0.6f;
constexpr float ROCK_HIT_HEIGHT = 0.8f; // tiles above the floor the rock still hits (a jump does not dodge it)
static_assert(
	WALK_SPEED * static_cast<float>(ROCK_WARN_MS + ROCK_FALL_MS) / 1000.f > 1.f + ROCK_GRAZE_HALF_WIDTH - 0.5f,
	"walking on from the cell's edge gets the player out of the graze before the rock lands"
);

// Dart trap (plan poison-dart-trap): a pressure plate under the player or a heavy monster (weight DART_PLATE_WEIGHT and
// up, MonsterKind::weight) clicks; DART_DELAY_MS later the holes in the back wall above it shoot DART_COUNT darts out
// across the corridor, DART_GAP_MS apart, one per hole. At the walking line (DART_HIT_DEPTH) they take the body in
// front of their hole at DART_HEIGHT over the floor: a jump at its top lets them pass under, small monsters too. Each
// one that hits deals DART_DAMAGE and poisons (medium). The plate re-arms DART_REARM_MS after the click, once nothing
// heavy stands on it.
constexpr int PLAYER_WEIGHT = 2;
constexpr int DART_PLATE_WEIGHT = 2;
constexpr int DART_DELAY_MS = 200;
constexpr int DART_COUNT = 3; // one per hole
constexpr int DART_GAP_MS = 160;
constexpr float DART_SPEED = 6.f; // tiles a second
constexpr float DART_HIT_DEPTH = 0.5f; // tiles out of the back wall: where the player and the monsters walk
constexpr float DART_HOLE_X[3] = {-0.22f, 0.f, 0.22f}; // the holes, from the plate's centre, in firing order
constexpr float DART_HEIGHT = 0.3f; // tiles over the floor
constexpr int DART_DAMAGE = 2;
constexpr float DART_PLATE_HALF_WIDTH = 0.27f; // the slab (mechanism.py): a centre over it presses it
constexpr float DART_HALF_WIDTH = 0.1f; // a dart takes a body whose box comes this near its hole
constexpr int DART_REARM_MS = 3000;

// Pressure plates (plan pressure-plate): the weight on one is the player's (PLAYER_WEIGHT), the monsters' and their
// bodies' (MonsterKind::weight, a body at least 1) whose centre is within PRESSURE_PLATE_HALF_WIDTH of the plate's,
// and the things in its cell: a stone, an emptied chest, a mirror. A held plate sinks PRESSURE_PLATE_SINK (as a dart
// plate); its plate gates move as a key gate (GATE_OPEN_MS), up and down.
constexpr int STONE_WEIGHT = 4;
constexpr int CHEST_WEIGHT = 2;
constexpr int MIRROR_WEIGHT = 2;
constexpr float PRESSURE_PLATE_HALF_WIDTH = 0.35f;

// Trapped chests (plan trapped-chests, ChestTrap in loot.h): the trap goes off as the lid lifts. The darts out of the
// lock hit the player who opens it (CHEST_DART_DAMAGE, cut by the trap ward, and medium poison); the gas hangs over
// the chest for CHEST_GAS_MS, and a player within CHEST_GAS_HALF_WIDTH of its middle takes weak poison every
// CHEST_GAS_POISON_MS (the first at once); the swarm is CHEST_SWARM_SIZE scarabs digging out round the chest.
constexpr int CHEST_DART_DAMAGE = 6; // the plate's three darts
constexpr int CHEST_GAS_MS = 4000;
constexpr int CHEST_GAS_POISON_MS = 1500;
constexpr float CHEST_GAS_HALF_WIDTH = 0.8f; // tiles
constexpr int CHEST_SWARM_SIZE = 3;
// The tell: a trapped container is drawn darker, at CHEST_TELL_SHADE, but only with the player within
// CHEST_TELL_NEAR tiles; it fades out to CHEST_TELL_FAR, so it never shows from afar.
constexpr float CHEST_TELL_SHADE = 0.62f;
constexpr float CHEST_TELL_NEAR = 0.8f;
constexpr float CHEST_TELL_FAR = 1.8f; // the view shows about two tiles either side of the player
// The skills' sight (plan heka-surplus): the Wisdom of Seshat puts a sealed room within SESHAT_ROOM_RADIUS cells on the
// draft map; Disarm trap draws a red contour round trapped containers and disguised mimics within DISARM_SEEN_FAR, a
// little farther than the chest's own tell, fading in over OUTLINE_FADE tiles.
constexpr int SESHAT_ROOM_RADIUS = 3;
constexpr float DISARM_SEEN_FAR = 2.6f;
constexpr float OUTLINE_FADE = 0.8f;
// Tomb curses (plan trapped-chests): smashing a living ushabti curses with fatigue, at this chance; the trap ward's
// chance stops it as a chest's trap.
constexpr int TOMB_CURSE_PERCENT = 25;

// Poison drips (plan poison-drip-trap): the puddle poisons (weak) whoever walks into it, once per entry, no damage,
// when their middle is within DRIP_PUDDLE_HALF_WIDTH of the cell's; a jump clears it. On the cycle the lump falls into
// it: whoever is in the cell while the venom falls (the out time of the wall spikes' cycle) takes DRIP_DROP_DAMAGE (the
// trap ward cuts it, not the poison) and medium poison, once per entry, a jump too.
constexpr float DRIP_PUDDLE_HALF_WIDTH = 0.35f;
constexpr int DRIP_DROP_DAMAGE = 8;

#endif
