# Giant bat: screech

Status: draft 2026-10-19, ready (needs nothing first). From the user's review: each giant gets an ability of its own,
not only bigger numbers.

## Idea

The giant bat screeches: a cone of sound that dazes the player.

* **Tell:** it stops in the air, its body shakes, a rising squeal, ~0.6 s. The player can step out of the cone or
  break line of sight.
* **Cone:** along its facing, ~4 tiles long, ~60°. Walls block it.
* **Daze:** ~1.5 s: the player walks at half speed, cannot sprint or jump, the view wobbles (a small camera roll and
  sway). Melee and the bow still work. No damage of its own.
* **On a ladder:** a dazed player lets go and falls (fall damage as today). The tall halls with bats are where it
  matters ([bats-high-ceiling](bats-high-ceiling.md)).
* Cooldown: default 6 s. One screech per bat in the air at a time.
* No resistance, no amulet. Journal note in the bat's tone: "It screamed. My ears rang, my legs forgot me, the
  ladder did not wait."

## Boss: the Bat of Shezmu

Boss rule (AGENTS.md: its `kin` is the bat; the giant bat's ability goes to the boss too, stronger): a wider cone
(~90°, 5 tiles), a 2 s daze. A dazed player bitten by Shezmu loses more: the bite heals Shezmu by its damage (the
butcher god drinks). Cooldown ~5 s.

## Tests

* Unit / sim: the cone hits in range and facing and not through walls, the daze slows and blocks sprint and jump,
  a dazed player on a ladder falls, Shezmu's bite heals him only while the player is dazed.
* Scenario with a screenshot: the tell and the wobbled view.
