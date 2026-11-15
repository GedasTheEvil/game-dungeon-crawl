# Soft landing at the bottom of drop shafts

Status: implemented 2026-11-16, waiting for the user's play-test. Was: draft 2026-11-15, size M, needs nothing first.

## Idea

The game has no fall damage, but many campaign levels drop the player down a shaft of several rows onto bare rock.
Put something soft at the bottom, fitting the zone, so the harmless fall looks believable. Side gain: a soft heap
tells the player "this drop is safe", which a death pit never shows.

## Plan

* A **landing heap**: a new floor decor (decor layer, like the water props, `level/decor.h`), visual only, no rule
  change. Not a new `Structure`: sand and half water already slow the player, and a heap must not.
* One look per zone (zone look table, `ZONE_LOOKS`), first guess, the implementer may change:
  * tomb entrance: drifted sand and rubble dust;
  * bat galleries: guano heap;
  * web halls: a sagging web net across the shaft foot (the player sinks into it, it springs back);
  * priests' quarter, serpent temple: reed mats and linen bundles;
  * scorpion nest: sand;
  * cisterns of Sobek: half water where it fits the level, else wet reeds;
  * Seth's desert, land of Sokar: a sand drift (not quicksand);
  * lake of fire: an ash heap;
  * necropolis: a pile of linen wrappings;
  * hall of judgment: a heap of ostrich feathers (Maat's feather).
* Where a shaft already ends in sand or half water, add nothing.
* Landing feedback on any fall of 2+ rows: a soft thud and a puff of the heap's matter (dust, ash, feathers,
  splash). On a bare floor (test levels, short drops) a plain thud, no puff.
* Placement: the level generator puts a heap under every drop of 3+ rows (`level_gen_layout.cpp`), and the level
  checker warns on a drop of 3+ rows onto a bare floor (the fall length from the path model, `level_walker.*`). Then
  fix every campaign level by hand to pass the checker (AGENTS.md: no warnings); do not regenerate levels.
* Monsters that fall (pushed, a jumper's leap) land on it the same way; no rule change for them.
* Models or textures: small, one per zone, built per [modeling.md](../modeling.md) or as a floor overlay texture like
  the sand edge (`tools/textures/`). Implementer's choice.
* Glossary rows: landing heap, (the thud and puff if named).

Open points (implementer's choice): the drop length that counts (3 rows proposed); a ladder shaft (the player can
climb down) gets a heap too or not (proposed: yes, any shaft the player can fall down).

## Play-test

* `bin/game --level 10` (web halls, several shafts): drop down a shaft; the foot has the zone's heap, a thud and a puff.
* One level each of the desert, lake of fire and necropolis: the heap fits the zone.
* A death pit has no heap.

## Result

* Seven placed-only props `heap_sand`, `heap_guano`, `heap_web`, `heap_reeds`, `heap_ash`, `heap_linen`,
  `heap_feathers` (`DECOR_HEAP_*`, `level/decor.h`; models in `tools/blender/models/decor.py`, editor icons), the
  zone's one by `heapOfZone` (`level/zone_look.h`): sand for the tomb entrance, the nest, the desert and Sokar; reeds
  for the priests' quarter, the serpent temple and the cisterns; the rest as proposed. Visual only, no rule.
* Checker (`checkLandings`): a fall of 3+ rows (a drop, a jump or a step whose landing is that much lower, from the
  search) onto a cell where a heap fits (`takesLandingHeap`: dry flat floor, no trap, nothing on the floor it would
  bury; a monster spawn and back-wall fixtures are fine) warns. Sand and half water count as soft. Ladders: the climb
  is not a fall, no heap (open point: no).
* Generator: `generateLevel` puts the zone's heap on every such cell (`LevelReport::bareLandings`) before it judges a
  candidate.
* Campaign: 85 heaps placed in 34 levels with the new `tools/level/lvledit.py prop` op; every level passes the checker
  with no warning; the reports are otherwise the same.
* Feedback: a fall of 2+ rows ends with a soft thud and a puff of the heap's matter on a heap (within half a tile:
  the walk key drifts the fall) or sand, a plain thud elsewhere, a splash in half water as before
  (`Dungeon::playerLanded`, `Grit::landing`, sounds `tools/audio/landing_sounds.py`).
* Left out: monsters get no thud or puff (their falls have no landing step; no rule change for them either way); the
  web net does not sag under the player (a still model).

## Play-test

* `bin/game --play tests/play/soft-landing.txt`: walk right off the ledge; a sand heap at the shaft's foot, a soft
  thud and a puff of sand. Walk on right and drop the 2 rows: a plain, harder thud, no puff.
* `bin/game --level 10` (web halls): the shafts end on a web net; `--level 43` (lake of fire) an ash heap; `--level
  49` (necropolis) linen: each fits its zone.
* A death pit (any spikes or death trap at a shaft's foot) has no heap.
