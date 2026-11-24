#ifndef QUICK_POTION_H
#define QUICK_POTION_H

#include "items.h"
#include <cstdint>
#include <optional>

// Health at or under this percent of max: the quick heal takes the large potion, if there is one (else the small one).
constexpr int QUICK_LARGE_HEALTH_PERCENT = 60;
// Stamina at or under this percent of max: the quick stamina key takes the large potion, if there is one.
constexpr int QUICK_LARGE_STAMINA_PERCENT = 20;
constexpr int QUICK_DRINK_COOLDOWN_MS = 1000; // mashing the key does not drink the whole stack

// The hotkeys H (health) and 0 (stamina) drink the small potion of their kind, unless the current value is at or under
// QUICK_LARGE_HEALTH_PERCENT / QUICK_LARGE_STAMINA_PERCENT of max and a large one is there.
enum class QuickKind : std::uint8_t { Health, Stamina };

// The potion to drink, nullopt when nothing is missing (current >= max) or none of that kind is left.
[[nodiscard]] std::optional<ItemKind> quickPotion(QuickKind kind, int current, int max, int smallCount, int largeCount);

#endif
