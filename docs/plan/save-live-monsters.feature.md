# A save keeps the live monsters health and the boss fight

Status: implemented 2026-11-14, waiting for the user's play-test. Was: draft 2026-11-13, size M, needs nothing first.

## Idea

A save holds the level's map, keys, explored cells and containers (`Dungeon::Dump`, `src/sim/dungeon_io.cpp:128`), not
the monsters. A load calls `clearMonsters` (`dungeon_io.cpp:98`), which also resets the boss fight
(`bossFight = BossFight{}`, `src/sim/dungeon_monsters.cpp:246`); every unslain monster comes back from its spawn tile
at full health when its tile comes into view (`SpawnMonster`, `dungeon_monsters.cpp:297`), and the summoned minions,
webs, blazes and doused torches are gone. `dungeon.h:558` says so: "Monsters are not saved: a load starts the fight
over."

It mattered less while a save took the menu (a slot save is allowed at any moment too) or the autosave, which waits for a safe moment
(`AutosaveMoment`, `Dungeon::PlayerInDanger`). Since plan quick-save (2026-10-29), F5 / F9 save and load in one key
press at any moment (`GameState::Quicksave`, `src/state/game_state.cpp:331`: only "alive" and "not won" are checked).
So in a fight:

1. A boss at a tenth of its health: F5, F9. It stands at its spawn tile at full health; its minions are gone.
2. A monster chasing the player: F5, F9. It is back at its spawn tile, at full health, not chasing.

Either the player gets a free escape (2) or loses all the damage done (1); both turn a quick save into a fight tool.

## Plan

Smallest change that removes both: keep each live, unslain, non-minion monster's health by its spawn cell, and the
boss fight's counters.

1. `Dungeon::Dump`: after the containers, a tagged line `monsters N` and N records `col spawnRow hp`, only for active,
   non-minion monsters below their max health (the others respawn as today). Then `boss summoned hatched firstLeft`
   when a boss fight runs.
2. `Dungeon::LoadDump`: read them (a save without the tag: none, as today) into a small table by spawn cell;
   `SpawnMonster` applies the saved health when it spawns that cell's monster, then forgets the entry.
3. The boss: its health from the table; `startBossFight` takes the saved counters, so the minions already summoned
   are not summoned again in full.
4. Position and AI state stay unsaved: the monster restarts at its spawn tile (a chase still ends on load; say so in
   the plan's Done or decide on step 5).
5. Optional (decide with the user): no quick save while `PlayerInDanger()` (the autosave's rule), with a toast "Not
   now: danger near". It would cover position too, at the cost of a refused key.

## Tests

* Unit (`SimWorld`): hit a monster to half health, `SaveAndLoad`, walk into view: it spawns with half health. A
  boss at a tenth: the same, and the summon counters carried.
* An old save (no `monsters` tag) loads as today.

## Done

Steps 1 to 4. A save holds `monsters: N col row hp ...` (the hurt, living, non-minion ones) and, while a boss lives,
`boss: summoned hatched firstLeft minions` (`Dungeon::dumpMonsters`, `loadMonsters`, `src/sim/dungeon_io.cpp`).
`SpawnMonster` gives a saved monster its health back (`Monster::RestoreHealth`); `startBossFight` takes the saved
counters, and its first minions are those alive at the save, up to `minAlive` (a hatcher's first ones still to come
count too). Position and AI state stay unsaved: a load still sends each monster back to its spawn tile, so a chase
ends on load. Step 5 (no quick save in danger) is not done: it is the user's call (open question below).
Tests: `tests/unit/save_live_monsters_test.cpp` (a rat, Babi, an old save without the tags).

## Play-test

* `bin/game --play tests/play/save-live-monsters.txt` (Babi's hall, `god`): hit Babi to about half, F5, F9: his
  health bar shows half, not full; the baboons alive at the save come back, no more.
* `bin/game --level 1`: hit a plain monster a few times, F5, F9, walk back to it: its bar shows the damage.

## Open questions

* Step 5 (refuse the quick save in danger) instead of, or as well as, saving the health: the user's call.
