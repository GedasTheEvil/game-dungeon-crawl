# Giant scorpion: pincer grab, then the sting

Status: draft 2026-10-19. Build together with [crocodile-hold-bite](crocodile-hold-bite.draft.opus.md): one hold
mechanic for both (the player held, actions locked, an end), not two. Whichever is done first builds it. From the
user's review: each giant gets an ability of its own.

## Idea

The giant scorpion grabs with its pincers and holds the player; then the tail strikes and never misses.

* **Tell:** pincers open wide, the tail curls up, ~0.5 s. Backing off or jumping away makes it miss.
* **Grab:** at 25% of its attacks (gameplay stream), in melee. The pincers do a small blunt hit.
* **Hold:** ~1 s. The player cannot walk, jump, sprint or drink; unlike the crocodile's hold, attacking does work and
  each hit on it shortens the hold by 0.25 s (mash to break free).
* **The sting:** at the hold's end, a sure hit: its sting's damage and a medium poison (as its sting today). Breaking
  free before the end skips it.
* In sand ([desert-chapter](desert-chapter.draft.opus.md)): it lurks under the surface like the sand worm, if that
  plan is in.
* Journal note: "Two claws to hold you, one tail to finish. Hit the claws."

## Boss: the scorpion queen

Boss rule (her `kin` is the scorpion): the same grab, stronger: a 1.5 s hold, the sting a strong poison, each hit
shortens it by 0.2 s only.

## Tests

* Unit / sim: the grab rolls on the gameplay stream, the hold locks movement and drink but not the attack, hits
  shorten it, the sting lands at the end and not after a break-free, the queen's numbers.
* Scenario with a screenshot: the tell, the held view.
