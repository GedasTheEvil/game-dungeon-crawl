# Living ushabti

Status: implemented 2026-10-19, waits for the user's play-test. From the lore review, picked by the user.

Play-test: `bin/game --play tests/play/living-ushabti.txt` (a hall of decor ushabti, one of them alive; walk east), or
`bin/game --level 11` (also in 12, 13 and 14).

## Idea

Ushabti are the small servant figures buried to work for the dead. The `ushabti` decor (`DECOR_NAMES`,
`src/world/decor.h`) comes alive: a new monster that looks like the decor until the player comes near, then stands
up and walks. Same family as the Anubis living statue ([egyptian-names](solved/egyptian-names.md)).

* Kind: small, slow walker, a few HP. Glazed faience shatters: **weak to blunt**, resists pierce. Journal note with humour: "It says 'here I am' on its chest. It means it."
  (the ushabti spell, Book of the Dead ch. 6).
* Lurk: idle as the decor figure (like the mimic's ambush), wakes in range.
* Placement: among real ushabti decor, so the player cannot tell; levels where ushabti decor appears.
* Anubis boss: summons ushabti instead of (or beside) mummies (`Summon::Coffin` → a new rise from the floor, or the
  existing one): implementer's choice.
* Glyph: pick a free one. Model: the ushabti decor model, rigged with a walk, rise and death (crumbles) clip.
* No boss has it as kin unless the Anubis boss's kin changes; then the boss rule applies.

## Done

* Kind `MonsterUshabti` (28, glyph `@`: the free letters are taken by the level sources' defs; `KINDS` in
  `src/world/monster_kinds.cpp`): the overseer of the servant figures, 0.28 tiles tall (twice the decor figures, half the
  player). Speed 6 (slower than the player), 40 HP, a hoe chop of 9 every 1.3 s (blunt 60 : pierce 40), threat 1.5, 900
  XP, levelgen difficulty 6-10. Faience: weak to blunt, resists pierce, tough against fire (kiln-fired: a new exception to
  the fire rule, in its unit test). Poison immune, weight 1.
* New `Locomotion::Figurine`: lurks (its idle clip, facing the camera) until the player is within `FIGURINE_WAKE_RANGE`
  (1.3) along its row or hits it, then plays its rise clip (`riseMs` 900) and walks. The rise uses `FLOATING_CLIPS`.
* The decor ushabti on its spawn cell (`scatterProps`, `DECOR_USHABTI`), at any depth, also once it is slain, as the coffin
  on a mummy's.
* Journal: the note ("... It says 'here I am' on its chest. It means it.") and a move, `CreatureMove::StepOff`.
* Anubis boss (kin unchanged): every other summon out of the wall is an ushabti (`BossRules::smallMinion` now also for
  `Summon::Wall`); summon cap 4 → 8 and 3 alive at most, so his 5 guards still come. Journal: "His statues and his
  servants step out of the stone at his call."
* Levels: one each in 11, 13 and 14, two in 12 (the tomb halls; levelcheck clean).
* Model `tools/blender/models/ushabti.py` (no rig, rigid parts; move, attack, die, idle, rise), sounds
  `tools/audio/ushabti_sounds.py` (wake, attack, die).
* Tests: `tests/unit/ushabti_test.cpp` (still until in range, steps off, follows slower than the player, bites; a hit wakes
  it, half from a point, double from a blow; the Anubis boss's second wall summon), a decor case in
  `tests/unit/decor_test.cpp`, `tests/scenarios/ushabti.txt` (idle among decor figures, rises, walks, chops, crumbles;
  screenshots).

## Open

* The idle ushabti stands on the walk line, a little in front of the decor figures of its cell (they stand by the back
  wall). A look at it in play decides whether that gives it away.

## Tests

* Unit / sim: wakes in range, walks, takes double blunt.
* Scenario with a screenshot: idle among decor ushabti, awake.
