# Skill: dodge

Play-test: `./game --play tests/play/skill-dodge.txt`

Status: draft 2026-10-10, implemented 2026-10-12 (see [Implemented](#implemented-2026-10-12)), waiting for play-test. From the user.

The skill is on the Heka screen already, sealed: implementing it clears its `sealed` in `SKILLS`
(`src/world/skills.cpp`) and checks its card text there.

## Idea

A new mechanic: with the skill, the player has a 10% chance to avoid a monster's attack entirely.

* Rolls on each monster hit on the player (melee bites, stings, spit, bolts): no damage, no poison. Traps, rock falls
  and darts: not dodged (default; implementer's choice).
* Feedback: a quick sidestep of the view (a small camera nudge), a whoosh sound, a "Dodged" status line or floating
  word.
* Rolls on the gameplay stream.
* Later ideas (not in this draft): a dodge amulet, dodge raised by agility.

## Decided (user, 2026-10-11)

* Dodge works everywhere: wading, on a ladder, mid-jump.

## Tests

* Unit / sim: about 10% of hits dodged (seeded), a dodged poisoner hit does not poison.

## Implemented (2026-10-12)

* `Player::Dodges(events, rng)` (`src/entities/player.cpp`): with the skill, rolls `DODGE_PERCENT` (10,
  `gameplay_config.h`) on the gameplay stream; a dodge says "Dodged" and plays `WorldSound::Dodge`. Called before the
  damage in `Monster::bite` (melee, bats, stings; no damage, no poison, no life steal, no journal hit), the charge
  hit (`monster_ai.cpp`) and the spit / bolt hit (`dungeon_arrows.cpp`). Traps, rock falls and darts are not dodged
  (default). Works everywhere, also in scenario god mode (so a test can see it).
* Sound: the existing `sounds/items/sling_swing.wav` as the whoosh (`SoundBank::dodge`).
* Skipped: the camera nudge (no camera hook from the world; the status line and the whoosh carry the feedback).
* `sealed` cleared; the card text stays "1 blow in 10 misses you".
* Tests: `tests/unit/skills_test.cpp` (the rate with a seeded stream, none without the skill, a plant's bites dodged
  in the sim).
