# Amulet of venom: poison monsters on hit

Status: draft 2026-10-07. Split off [amulets.md](solved/amulets.md); for later.

## Idea

* An amulet (tiered like the others) that gives the player's hits a chance to poison the monster: lesser 10%,
  grand 30% (minor and normal in between, open).
* Builds on [monster-poison](monster-poison.md) (done): `Monster::TakePoison(tier, true, rng)` on a hit.
* The journal's poison resistance line comes with it (left open there).

## Open

* Minor and normal chances.
* The tier the amulet's poison has (per amulet tier?). How poison works on a monster: [monster-poison](monster-poison.md).
* Name: "venom" to tell it from the poison resistance amulet.
