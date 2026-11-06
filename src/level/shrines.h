#ifndef SHRINES_H
#define SHRINES_H

#include "decor.h"
#include <cstdint>
#include <optional>

// The shrines of the gods (plans shrine-of-isis, shrines-of-the-gods): placed props, a naos with the god's statue in
// it. A prayer (interact, PlayerStats::Pray) gives the god's blessing for a while, as often as the player asks. The
// blessings of different gods run at once; a prayer at the same god starts its time again. They end on death.
// Saved by place: append only, never reorder.
enum class God : std::uint8_t { Isis, Montu, Ptah, Amun, Serket };
constexpr int GOD_COUNT = 5;

constexpr int BLESSING_MS = 120000; // every blessing: 2 minutes (the Isis ward: ISIS_WARD_MS, curses.h)
constexpr int MONTU_MIGHT = 5; // the Fury of Montu: Might (the grand Amulet of Strength gives 6, a level 1)
constexpr int PTAH_ARMOR = 5; // the Shield of Ptah: Armor
constexpr int AMUN_MAX_HP_PERCENT = 20; // the Grace of Amun: max HP, after a full heal

struct GodDef {
	const char* name; // "Montu"
	int decor; // the god's shrine (decor.h)
	const char* blessing; // the HUD's, the journal's and the glossary's: "Fury of Montu"
	const char* endLine; // the status line when the blessing runs out
};

constexpr GodDef GODS[GOD_COUNT] = {
	{"Isis", DECOR_SHRINE_ISIS, "Protection of Isis", "The Protection of Isis fades"},
	{"Montu", DECOR_SHRINE_MONTU, "Fury of Montu", "The Fury of Montu fades"},
	{"Ptah", DECOR_SHRINE_PTAH, "Shield of Ptah", "The Shield of Ptah fades"},
	{"Amun", DECOR_SHRINE_AMUN, "Grace of Amun", "The Grace of Amun fades"},
	{"Serket", DECOR_SHRINE_SERKET, "Shelter of Serket", "The Shelter of Serket fades"},
};

[[nodiscard]] constexpr const GodDef& godDef(God g) { return GODS[static_cast<int>(g)]; }

// The god whose shrine the prop is; none for any other prop.
[[nodiscard]] constexpr std::optional<God> shrineGod(int decor) {
	for (int g = 0; g < GOD_COUNT; g++)
		if (GODS[g].decor == decor)
			return static_cast<God>(g);
	return std::nullopt;
}

#endif
