#ifndef QUICK_POTION_H
#define QUICK_POTION_H

#include "items.h"
#include <cstdint>
#include <optional>

// Health under this percent of max after the small potion: the quick heal takes the large one, if there is one.
constexpr int QUICK_HEAL_DANGER_PERCENT = 35;
constexpr int QUICK_DRINK_COOLDOWN_MS = 1000; // mashing the key does not drink the whole stack

// The hotkeys H (health) and 0 (stamina) drink the weakest potion of their kind that gets the player out of danger:
// the small one, unless it would leave them in danger and a large one is there. Out of danger means health at or above
// QUICK_HEAL_DANGER_PERCENT of max, stamina enough for a jump (JUMP_STAMINA_COST).
enum class QuickKind : std::uint8_t { Health, Stamina };

// The potion to drink, nullopt when nothing is missing (current >= max) or none of that kind is left.
[[nodiscard]] std::optional<ItemKind> quickPotion(QuickKind kind, int current, int max, int smallCount, int largeCount);

#endif
