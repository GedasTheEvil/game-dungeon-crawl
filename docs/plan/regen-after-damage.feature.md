# Regeneration: none while burning, none for 5 s after any damage

Status: implemented 2026-11-16, waiting for the user's play-test. Was: draft 2026-11-16, size S, needs nothing first.

## Idea

No "auto-healing" during a fight. Today `PlayerStats::Regenerate` (`src/level/player_stats.cpp`) stops the HP regen of
the rings (garnet, sky-iron; "also in a fight") only when poisoned or bleeding, and the regeneration amulet when
poisoned, bleeding or not `Dungeon::PlayerSafe` (which already includes burning). The user's rules:

1. Burning stops all HP regeneration (the rings too).
2. General rule: no HP regeneration for 5 s after the player takes any damage (a monster hit, a shot, a trap, poison,
   bleeding or burn ticks, a fall, ...).

## Plan

* `REGEN_DAMAGE_PAUSE_MS = 5000` (named constant). The player's damage path sets a "no regen until" time on the
  `GameClock` (one place every HP loss goes through; find it, e.g. `Vitals`); a hit that the amulets or a resistance
  bring to 0 HP is no damage.
* `Regenerate`: `healing` is also false while burning and until that time passes; the ring and amulet carries reset as
  they do today when healing is false.
* Not saved (seconds). Stamina regeneration is unchanged. Potions and other heals are unchanged (only regeneration).
* Monsters: if any monster regenerates HP, the same two rules apply to it; else nothing to do.
* Update texts that say "also in a fight" (ring descriptions, docs, glossary row for regeneration if any): rings now
  heal in a fight only while not hit for 5 s.
* Unit test (`tests/unit/`): rings heal; a hit stops it for 5 s, then it heals again; burning stops it.

## Play-test

* `bin/game --play tests/play/regen-after-damage.txt`: garnet ring and a regeneration amulet worn, HP low (30 off), a camel spider and a mummy near.
* Let it hit you: HP does not rise for 5 s after each hit, then rises again.
* Stand in fire: no HP regeneration while burning.
