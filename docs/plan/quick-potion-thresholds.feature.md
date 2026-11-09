# Quick potion picks the large one at 60% health, 20% stamina

Status: implemented 2026-11-09, waiting for the user's play-test. Was: draft 2026-11-09, size S, needs nothing first.

## Idea

The HUD's quick healing and stamina slots (keys H and 0) suggest the potion the key would drink (`quickPotion`,
`src/level/quick_potion.cpp`; `Inventory::QuickChoice`). Today the large one is picked only when the small one would
leave the player in danger: health under `QUICK_HEAL_DANGER_PERCENT` (35%) of max after the small potion, stamina under
a jump's cost (`JUMP_STAMINA_COST`) after it. The user wants simpler, earlier thresholds on the current value:

* health at or under 60% of max: offer the large healing potion;
* stamina at or under 20% of max: offer the large stamina potion.

## Plan

* `quickPotion`: with both sizes held, large when `current * 100 <= max * 60` (health) / `max * 20` (stamina), else
  small. One held size only, full, none left: unchanged.
* Replace `QUICK_HEAL_DANGER_PERCENT` with two constants (`QUICK_LARGE_HEALTH_PERCENT = 60`,
  `QUICK_LARGE_STAMINA_PERCENT = 20`); update the comments in `quick_potion.h`.
* The HUD slot shows the suggestion and the key drinks it (both go through `quickPotion`): check both follow.
* Update the unit tests of `quickPotion` (grep `quickPotion` in `tests/unit/`) to the new edges: 60% / 61% health,
  20% / 21% stamina.
* Docs that state the old rule (grep `QUICK_HEAL_DANGER_PERCENT`, "danger" in `docs/`): follow it.

## Play-test

* `bin/game --play tests/play/quick-potion-thresholds.txt` (3 each of the small and large healing and stamina potions;
  health lowered by 40 and stamina by 80 of 100): the healing slot shows the large potion at 60% health and below, the
  small one above (drink with H: the slot flips to the small one at 61%+).
* Stamina: the slot shows the large potion at 20% and below, the small one above (stamina regenerates; 0 drinks what
  the slot shows). Command `tire N` takes N stamina off.
