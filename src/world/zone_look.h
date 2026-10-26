#ifndef ZONE_LOOK_H
#define ZONE_LOOK_H

// How each zone looks and sounds (plan zone-looks): its decor set, light, fog, torches, ambient loop and the guardian
// its banner names. The stone of the walls is the zone's (ZoneInfo, campaign.h; scatterSurfaces). A level is in the
// zone its file names (LevelMeta); one in none (generated, test) looks as the tombs always did (DEFAULT_LOOK).

#include <cstdint>
#include "campaign.h"
#include "decor.h"

struct ZoneLook {
	// Per prop and decal, a factor on its tier's weight (decor.h): 1 as anywhere, 0 never, more for the zone's own.
	// The tiers still decide what is unlocked.
	uint8_t props[DECOR_SCATTERED];
	uint8_t decals[DECAL_COUNT];
	float ambient[3]; // the light where no torch reaches (Lighting)
	float fog[3]; // the haze colour far from the camera
	float fogDensity; // per world unit past FOG_START; 0: no haze
	uint32_t torchPercent; // chance of a wall torch on a free cell (scatterTorches)
	const char* ambience; // media/sounds/ambience/<name>.wav, looped under the effects; nullptr: silence
	const char* guardian; // the zone banner's guardian line, nullptr on a bossless zone
};

constexpr float FOG_START = 55.f; // world units from the camera: the player stands about here

// Props: web, pottery, canopic, rubble, sand, skeleton, brazier, lamp, scrolls, ushabti, cat, jackal, osiris, Thoth,
// (3 Thoth variants, never picked), sarcophagus. Decals: crack, impact crack, floor crack, vines, flowering vines,
// roots, seepage, glyph column, glyph row, cartouche, eye of Horus, winged sun, papyrus, dry grass, creeper, moss.
constexpr ZoneLook DEFAULT_LOOK = {
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	{0.09f, 0.085f, 0.1f},
	{0.f, 0.f, 0.f},
	0.f,
	25,
	nullptr,
	nullptr
};

// By ZoneId, all twelve.
constexpr ZoneLook ZONE_LOOKS[ZONE_ID_COUNT] = {
	// Tomb entrance: cave and first tunnels, webs and the dead of the robbers.
	{{2, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
	 {1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	 {0.09f, 0.085f, 0.1f},
	 {0.05f, 0.045f, 0.05f},
	 0.004f,
	 25,
	 "tomb",
	 "Khepri, the scarab who rolls the sun"},
	// Bat galleries: roots and seepage from above, rock falls.
	{{1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
	 {1, 1, 1, 2, 1, 3, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1},
	 {0.08f, 0.08f, 0.1f},
	 {0.04f, 0.045f, 0.06f},
	 0.006f,
	 22,
	 "bats",
	 "the Bat of Shezmu, the butcher of the gods"},
	// Web halls: dark, webs everywhere, few fires (the spiders shun them).
	{{4, 1, 0, 1, 1, 3, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1},
	 {2, 2, 2, 0, 0, 3, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0},
	 {0.045f, 0.045f, 0.06f},
	 {0.02f, 0.02f, 0.03f},
	 0.01f,
	 8,
	 "webs",
	 "the spider queen of the webs"},
	// Priests' quarter: the priests' writing, lamps, braziers, ushabti and Thoth.
	{{0, 1, 2, 0, 0, 1, 2, 3, 3, 2, 1, 1, 1, 2, 0, 0, 0, 1},
	 {1, 0, 0, 0, 0, 0, 0, 3, 3, 3, 2, 2, 0, 0, 0, 0},
	 {0.11f, 0.095f, 0.08f},
	 {0.07f, 0.055f, 0.04f},
	 0.003f,
	 35,
	 "temple",
	 "the High Priest of Anubis"},
	// Scorpion nest: dry, cracked, sand blown in, the bones of the stung.
	{{1, 1, 1, 2, 2, 3, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1},
	 {2, 2, 2, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 2, 0, 0},
	 {0.1f, 0.085f, 0.07f},
	 {0.08f, 0.06f, 0.04f},
	 0.005f,
	 20,
	 "nest",
	 "the scorpion queen, the daughter of Serqet"},
	// Serpent temple: braziers, cats against the serpents, the eye of Horus.
	{{0, 1, 1, 1, 1, 1, 2, 1, 1, 1, 3, 1, 1, 1, 0, 0, 0, 1},
	 {1, 1, 1, 1, 1, 1, 1, 2, 2, 1, 3, 1, 0, 1, 2, 1},
	 {0.08f, 0.1f, 0.08f},
	 {0.03f, 0.05f, 0.035f},
	 0.005f,
	 25,
	 "serpent",
	 "Apep, the serpent of chaos"},
	// Cisterns of Sobek: damp, moss, papyrus, jars at the water.
	{{0, 3, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1},
	 {1, 0, 0, 2, 2, 1, 4, 1, 1, 1, 1, 0, 3, 0, 2, 4},
	 {0.07f, 0.09f, 0.11f},
	 {0.04f, 0.07f, 0.08f},
	 0.008f,
	 20,
	 "cisterns",
	 "Sobek, the crocodile of the Nile"},
	// Seth's desert: sand and the dead in it; daylight shafts, few torches.
	{{0, 2, 1, 2, 4, 2, 1, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1},
	 {2, 1, 1, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 3, 0, 0},
	 {0.12f, 0.1f, 0.075f},
	 {0.16f, 0.12f, 0.08f},
	 0.005f,
	 15,
	 "desert",
	 "Seth, the red lord of the desert"},
	// Land of Sokar: deeper sand, more bones, darker.
	{{0, 1, 0, 2, 4, 3, 1, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1},
	 {3, 2, 2, 0, 0, 1, 0, 1, 1, 1, 1, 0, 0, 2, 0, 0},
	 {0.1f, 0.085f, 0.065f},
	 {0.13f, 0.1f, 0.065f},
	 0.008f,
	 12,
	 "sokar",
	 "the great worm of Sokar"},
	// Lake of Fire: braziers, Thoth's baboons at the gate, scorched rock.
	{{0, 0, 0, 2, 0, 2, 4, 1, 0, 0, 0, 1, 1, 3, 0, 0, 0, 1},
	 {3, 2, 3, 0, 0, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0},
	 {0.16f, 0.05f, 0.03f},
	 {0.12f, 0.03f, 0.01f},
	 0.007f,
	 10,
	 "fire",
	 "Babi, the eater of hearts"},
	// Necropolis: coffins, jars of the dead, guardians, writing.
	{{1, 1, 4, 1, 0, 1, 1, 1, 1, 2, 1, 2, 2, 1, 0, 0, 0, 4},
	 {1, 1, 1, 0, 0, 1, 1, 3, 3, 3, 1, 1, 0, 0, 0, 0},
	 {0.065f, 0.07f, 0.09f},
	 {0.04f, 0.045f, 0.06f},
	 0.007f,
	 20,
	 "necropolis",
	 nullptr},
	// Hall of Judgment: the gods, Thoth the scribe, many fires, every wall written.
	{{0, 1, 1, 0, 0, 0, 2, 2, 1, 1, 1, 3, 3, 2, 0, 0, 0, 1},
	 {0, 0, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 0, 0, 0, 0},
	 {0.1f, 0.09f, 0.07f},
	 {0.08f, 0.065f, 0.035f},
	 0.003f,
	 35,
	 "judgment",
	 "Anubis, who weighs the heart"},
};

[[nodiscard]] constexpr const ZoneLook& zoneLook(ZoneId id) { return ZONE_LOOKS[static_cast<int>(id)]; }

// The look of a level in this zone (LevelMeta::zone), DEFAULT_LOOK in none.
[[nodiscard]] constexpr const ZoneLook& levelLook(int zone) {
	return isZone(zone) ? zoneLook(static_cast<ZoneId>(zone)) : DEFAULT_LOOK;
}

#endif
