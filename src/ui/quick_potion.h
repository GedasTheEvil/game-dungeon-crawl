#ifndef QUICK_POTION_H
#define QUICK_POTION_H

#include <cstdint>

// Potion strengths, in percent of the player's max health or max stamina.
namespace PotionEffect {
constexpr int SMALL_HEAL_PERCENT = 25;
constexpr int LARGE_HEAL_PERCENT = 50;
constexpr int SMALL_STAMINA_PERCENT = 50;
constexpr int LARGE_STAMINA_PERCENT = 100;
} // namespace PotionEffect

// Health under this percent of max after the small potion: the quick heal takes the large one, if there is one.
constexpr int QUICK_HEAL_DANGER_PERCENT = 35;
constexpr int QUICK_DRINK_COOLDOWN_MS = 1000; // mashing the key does not drink the whole stack

// The hotkeys H (health) and 0 (stamina) drink the weakest potion of their kind that gets the player out of danger:
// the small one, unless it would leave them in danger and a large one is there. Out of danger means health at or above
// QUICK_HEAL_DANGER_PERCENT of max, stamina enough for a jump (JUMP_STAMINA_COST).
enum class QuickKind : std::uint8_t { Health, Stamina };
constexpr int NO_POTION = -1;

// PotionId of the potion to drink, NO_POTION when nothing is missing (current >= max) or none of that kind is left.
[[nodiscard]] int quickPotion(QuickKind kind, int current, int max, int smallCount, int largeCount);

#endif
