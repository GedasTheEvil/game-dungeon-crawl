# Hippo

Status: draft 2026-10-10, ready (needs nothing first). From the lore review, picked by the user.

## Idea

The hippo, the deadliest animal of the Nile. A water monster: lies submerged in half water (eyes and ears only, like
the crocodile's lurk), then charges along its row through the water.

* Locomotion: submerged lurk, swimmer (`Wading::Swimmer`), and a **charge** like Sobek's (`Charge`): a wind-up, a
  rush through the player, stopped by walls. In half water the wading player cannot jump over it.
* Heavy: high HP, blunt bite, resists blunt; slow on land (it may leave the water, slower).
* Levels: flooded levels after the crocodile's; `levelcheck` requires it in or next to water like the crocodile.
* Boss: none has it as kin. Optional (implementer's choice, a separate step): an Ammit or a great hippo boss later;
  if one gets the hippo as kin, its charge is longer and faster (boss rule).
* Journal: "Taweret's people. On the walls they protect mothers. In the water they protect nothing."
* Glyph: pick a free one. Model: procedural, clips: idle (submerged), rise, wind-up, charge, bite, death.

## Tests

* Unit / sim: lurks, charges along the row, stops at a wall, swims faster in half water.
* Scenario with a screenshot: submerged, charging.
