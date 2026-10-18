# Camel spider

Status: implemented 2026-10-18, waits for the user's play-test. From the lore review, picked by the user.

Play-test: `bin/game --play tests/play/camel-spider.txt` (a hall with two torches; stand in their light, step out of
it, step back in), or `bin/game --level 4` (also in 6, 7 and 8).

## Idea

The camel spider (solifuge) of the desert: fast, big jaws, the stuff of soldiers' stories. It also gives the
`web` decor a joke, because camel spiders spin no webs.

* Kind: a fast walker, low HP, slash bite. Folklore: it chases your shadow. Mechanic: it runs at the player in the
  dark and **flees from light** (torches, braziers nearby): it will not step into a lit cell's range
  (implementer's choice how light is measured; braziers and wall torches as sources).
* Not a poisoner (real ones are not venomous): journal note says so, with humour: "Not venomous, they say. It does
  not need to be."; and on the webs: "It spins no webs. So who made these?"
* Levels: early-middle desert-feel levels (sand floors).
* Boss: none has it as kin.
* Glyph: pick a free one. Model: procedural (8 legs plus long pedipalps, so it looks ten-legged), clips: walk, bite, death.

## Done

* Kind `MonsterCamelSpider` (26, glyph `h`, `KINDS` in `src/world/monster_kinds.cpp`): speed 28 (~1.4 tiles a second,
  faster than the giant rat and the player's walk), 26 HP, 5 damage every 0.55 s, slash 80 : pierce 20, weak to the
  blade and fire, resists the point, threat 2.5, 1100 XP; levelgen from difficulty 3 to 8. No poison, no boss kin.
* Light (`MonsterKind::shunsLight`, `LIGHT_SHUN_CELLS` in `gameplay_config.h`): a cell is lit within 1 cell along its row
  of a flame among the decorations, the same ones the renderer lights: a wall torch, a brazier, an oil lamp
  (`Dungeon::Lit`, `flamesAt`). The spider does not step into a lit cell (`Dungeon::walkerStops`); a lit cell between
  it and the player, or round the player, means it cannot reach them, so the coward's rule takes over: it runs out of
  the bow's range along its row and waits there (its idle clip) until the player leaves the light. Caught in the light
  (placed there), it runs for the nearer dark (`Dungeon::wayOutOfLight`). A player at the very edge of the light is
  still in its bite reach from the dark.
* Journal: the note ("... Not venomous, they say. It does not need to be. It spins no webs. So who made these?") and a
  move, `CreatureMove::ShunLight` ("Will not come into the torchlight. It waits in the dark until I leave it.").
* Levels: one each in 4, 6, 7 and 8 (sand-floored caves and tunnels, after the scorpions' first levels). Levelcheck
  clean.
* Model `tools/blender/models/camel_spider.py` (on `scorpion.py`'s rig): head plate, sawing chelicerae with hinged
  jaws, fat abdomen, eight hairy legs, two long pedipalps with suckers; clips walk, attack, die and idle. Sounds
  `tools/audio/camel_spider_sounds.py`.
* Scenario command `torch COL on|off` (and `Dungeon::PlaceTorch`), for the tests and the play-test script.
* Tests: `tests/unit/camel_spider_test.cpp` (the lit cells; chases and bites in the dark; keeps out of the light round
  the player and comes once it is gone; turns at a torch lit in its face; a lit stretch is a wall; runs out of the
  light), `tests/scenarios/camel_spider.txt` (waits, runs, bites, dies; screenshots).

## Open

* Fire on the floor (a burning monster, a fire arrow) gives no light to it yet; only the decorations' flames.
* The player's own light does not count (it would keep it off for ever).
