#ifndef GAMEPLAY_WORLD_H
#define GAMEPLAY_WORLD_H

// An inscription translated gives this part of a riddle's reward (plan inscription-xp).
constexpr int INSCRIPTION_XP_PART = 3;

// The death screen (plan autosave) comes up once the death clip has played, so the death is seen.
constexpr int DEATH_SCREEN_DELAY_MS = 1200;

constexpr int GATE_TRAVEL_FADE_MS = 300; // the view fades out into a gate's plasma colour, then back in at the arrival
// The ring transporter (plan ring-transporter): RING_COUNT rings drop from the ceiling one after another,
// RING_STAGGER_MS apart, RING_FALL_MS each, and hum for RING_HOLD_MS; the view flashes white for RING_FLASH_MS, the
// player is moved, it flashes back; the rings rise at the partner the same way, the top one first.
constexpr int RING_COUNT = 5;
constexpr int RING_STAGGER_MS = 150;
constexpr int RING_FALL_MS = 400;
constexpr int RING_HOLD_MS = 250;
constexpr int RING_FLASH_MS = 180;
constexpr int RINGS_DOWN_MS = (RING_COUNT - 1) * RING_STAGGER_MS + RING_FALL_MS + RING_HOLD_MS;
constexpr int RINGS_UP_MS = (RING_COUNT - 1) * RING_STAGGER_MS + RING_FALL_MS;
// The Stargate (plan stargate-chamber): dialled, its inner track turns for STARGATE_TURN_MS and a chevron locks, one
// every STARGATE_CHEVRON_MS; the seventh will not lock: it flickers for STARGATE_FAIL_MS, then the six go dark over
// STARGATE_FADE_MS.
constexpr int STARGATE_CHEVRONS = 7;
constexpr int STARGATE_CHEVRON_MS = 700;
constexpr int STARGATE_TURN_MS = 420;
constexpr int STARGATE_FAIL_MS = 1400;
constexpr int STARGATE_FADE_MS = 800;
// When the seventh fails, and when the dial is over.
constexpr int STARGATE_FAIL_AT_MS = (STARGATE_CHEVRONS - 1) * STARGATE_CHEVRON_MS + STARGATE_TURN_MS;
constexpr int STARGATE_DIAL_MS = STARGATE_FAIL_AT_MS + STARGATE_FAIL_MS + STARGATE_FADE_MS;
// The first dial of the Stargate, and the first press on its dial device, each pay at least this much XP (a riddle's
// worth if that is more).
constexpr int STARGATE_MIN_XP = 20000;
// A sealed name gate's plasma (plan secret-name-gates) breathes down to NAME_GATE_MIN_ALPHA and back once per
// NAME_GATE_BREATH_MS; opened, it shimmers for NAME_GATE_SHIMMER_MS, then holds steady.
constexpr float NAME_GATE_BREATH_MS = 1500.f;
constexpr float NAME_GATE_MIN_ALPHA = 0.15f;
constexpr float NAME_GATE_SHIMMER_MS = 900.f;

// Keys, gates and levers (dungeon_mechanisms.cpp).
constexpr int GATE_OPEN_MS = 1200; // the gate slides up into the ceiling, passable once it is up
constexpr float GATE_APPROACH = 0.45f; // tiles from the gate at which a held key opens it
constexpr int LOCKED_HINT_INTERVAL_MS = 3000; // "needs the X key" at most this often
constexpr float KEY_SPIN_DEG_PER_MS = 0.12f;

// Cut-scenes (cut_scene_dungeon.cpp, plan cinematic-effects): a lever or a boss's death opens a gate off the screen,
// the camera goes to show it. The world stands still meanwhile.
constexpr int CUTSCENE_PAN_MS = 600; // the camera eases from the player to a gate, from one gate to the next, back
constexpr int CUTSCENE_HOLD_MS = GATE_OPEN_MS + 200; // it stays on a gate while it opens
constexpr int CUTSCENE_BARS_MS = 300; // the letterbox bars and the tint come in, and go at the end
constexpr int CUTSCENE_MAX_STOPS = 2; // the gates shown, nearest first

// Sunlight mirrors (plan sunlight-mirrors): a turned mirror swings to its new way in MIRROR_TURN_MS. A sun burner (the
// mummies) in a beam burns as from a hit with SUN_BURN_FIRE fire (Burning: a fifth of it a second, BURN_MS after it
// steps out). A sun disc just lit flares for SUN_DISC_FLARE_MS, then glows steady.
constexpr int MIRROR_TURN_MS = 350;
constexpr int SUN_DISC_FLARE_MS = 1500;
constexpr double SUN_BURN_FIRE = 75.0;

#endif
