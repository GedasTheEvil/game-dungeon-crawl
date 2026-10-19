# Crocodile bite with a hold

Status: solved 2026-10-19 (implemented and play-tested; Sobek: `bin/game --level 25`). Draft 2026-10-05, refined 2026-10-09. Split off
[crocodiles-and-flooded-cells.md](crocodiles-and-flooded-cells.md). One hold mechanic with
[giant-scorpion-grab](giant-scorpion-grab.md), built together.

## Idea

The crocodile's bite grabs the player: a hold, a death roll.

## Decided (2026-10-09)

* **Where:** only while the player stands in water (a flooded cell). On land the crocodile bites as today.
* **When:** a bite turns into a hold at 30%, rolled on the gameplay stream.
* **Telegraphed:** before a grab the crocodile opens its jaws wide and waits 0.5 s, then lunges. The player can back
  off or jump away; a lunge that misses does nothing. A plain bite stays as it is.
* **The hold:** the grab does the bite's damage, then the death roll: a tick every 0.8 s of 40% of the bite's damage
  (about 10), 3 ticks, 2.4 s in all. Armour counts on each tick. Then the crocodile lets go.
* **While held:** the player can do nothing: no walking, jumping, sprinting, attacking or drinking (the quick-drink
  keys and the inventory too). No way to break free: the hold is short instead.
* The crocodile dying while it holds (a venom tick) ends the hold.
* No amulet against it.
* **Sobek** (boss rule, AGENTS.md: its `kin` is the crocodile): the same hold, stronger. A grab at 40%, the same 0.5 s
  tell (shorter is unfair at its damage), 4 ticks of 40% of its bite, 3.2 s in all. Only while the player stands in
  water, as the crocodile's. A charge (`Charge`) never grabs: only the bite does.

## Implemented (2026-10-19)

* One mechanic for both plans: `HoldRules` in the kind's row (`MonsterKind::hold`, `src/world/monster_kinds.cpp`), the
  monster's `Grab` state (`src/entities/monster_grab.{h,cpp}`), the player locked by `Dungeon::PlayerHeldBy`
  (`Move`, `StartJump`, `AttackAllowed`; the drinks, the interact key and the screens in `src/input/input.cpp`; the
  walk keys still turn the player). Numbers as decided; the tell is `GRAB_TELL_MS`, a jump `GRAB_JUMP_CLEAR` up makes
  the lunge miss. The grab can be dodged (the Dodge skill), the ticks cannot.
* Look, no new Blender clips: the attack clip held at its widest gape (frame 8) for the tell, the jaws shut (frame
  13) in the hold; the death roll turns the model over along its body, a whole turn with each tick
  (`Monster::HoldRoll`); the held view rolls with it (`Dungeon::HeldViewRoll`, with the motion effects on). Status
  line "Caught in its jaws"; journal move `CreatureMove::Grab`.
* Tests: `tests/unit/hold_test.cpp`; scenario with screenshots `tests/scenarios/solved/crocodile_hold.txt` (scenario command
  `grab`, field `held`).
* Balance: the hold is short and only in water; the threat and the checker are unchanged (noted in
  [monster-balance](../monster-balance.draft.opus.md)).

## Open (for the implementer, done above)

* Look: the crocodile's and Sobek's open-jaws tell, the grab and the roll (Blender,
  [../../modeling.md](../../modeling.md)); the
  player's held view (camera shake or roll).
* The checker and the balance: the hold is short and only in water, so likely nothing. Note it with the crocodile's
  half-water weight in [monster-balance](../monster-balance.draft.opus.md).
