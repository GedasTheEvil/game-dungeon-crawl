# Giant scorpion: pincer grab, then the sting

Status: solved 2026-10-19 (implemented and play-tested; the queen: `bin/game --level 15`). Built together with [crocodile-hold-bite](crocodile-hold-bite.md): one hold
mechanic for both (see there). From the user's review: each giant gets an ability of its own. Left out: lurking in
sand, until [desert-chapter](desert-chapter.md) is in.

## Idea

The giant scorpion grabs with its pincers and holds the player; then the tail strikes and never misses.

* **Tell:** pincers open wide, the tail curls up, ~0.5 s. Backing off or jumping away makes it miss.
* **Grab:** at 25% of its attacks (gameplay stream), in melee. The pincers do a small blunt hit.
* **Hold:** ~1 s. The player cannot walk, jump, sprint or drink; unlike the crocodile's hold, attacking does work and
  each hit on it shortens the hold by 0.25 s (mash to break free).
* **The sting:** at the hold's end, a sure hit: its sting's damage and a medium poison (as its sting today). Breaking
  free before the end skips it.
* In sand ([desert-chapter](desert-chapter.md)): it lurks under the surface like the sand worm, if that
  plan is in.
* Journal note: "Two claws to hold you, one tail to finish. Hit the claws."

## Boss: the scorpion queen

Boss rule (her `kin` is the scorpion): the same grab, stronger: a 1.5 s hold, the sting a strong poison, each hit
shortens it by 0.2 s only.

## Implemented (2026-10-19)

* `GIANT_SCORPION_HOLD`, `SCORPION_QUEEN_HOLD` (`src/world/monster_kinds.cpp`): 25% of its bites, the pincers 30% of
  its damage, blunt; the hold 1 s (the queen 1.5 s), each hit 0.25 s off (0.2 s); the sting its damage and its poison
  (medium, the queen's strong). A hit that brings the end to now or before frees the player ("You break free"); so
  one hit in the last 0.25 s is enough, two from halfway.
* Look: the attack clip held at frame 3 (claws wide, the tail up) for the tell, frame 8 (pinched, the tail cocked)
  in the hold, then the strike plays on; the held view shakes. Status line "Caught in its claws: hit them!".
* Scenario `tests/scenarios/solved/giant_scorpion_grab.txt` (level `tests/levels/giant_scorpion`), unit tests in
  `tests/unit/hold_test.cpp`.

## Tests

* Unit / sim: the grab rolls on the gameplay stream, the hold locks movement and drink but not the attack, hits
  shorten it, the sting lands at the end and not after a break-free, the queen's numbers.
* Scenario with a screenshot: the tell, the held view.
