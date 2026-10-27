# Loading per level

Status: draft 2026-10-22, from the user. Needs first: [faster-loading](../solved/faster-loading.md). Then
measure again: if startup is fast enough, drop this plan (ask the user).
Won't do 2026-10-27: faster-loading did better than expected, startup is fast enough.

## Idea

Load only the assets the level needs, not everything at start. Each level change loads only the missing ones.
Loaded assets stay loaded; nothing is unloaded.

## Scope: monster models only

Monsters take about 65 % of the asset load time (measured in faster-loading), and their models are 139 of the 153 MB
of models. Items, decor, mechanisms, fonts, sounds and UI stay in the start load: they are small, and almost every
level uses them.

## What a level needs

Missing a model means a crash or an invisible monster. The set to load for a level:

* every monster spawn in the level (`spawnType`, `src/world/level.h`), also the slain ones in a save;
* what they can bring in: boss summons (Khepri: scarabs, Shezmu: bats, the ushabti from the wall, `Summon::Wall`),
  chest-trap and mimic spawns (`src/world/dungeon_chest_traps.cpp`), anything else that calls `SpawnMonster` or
  makes a monster at runtime: grep all of them, and derive the set from data (`MonsterKind` fields), not from a
  hand-written list;
* the kin each one shares its clips with (`loadMonsterTypes`: same `model` and the same `ClipFiles`): the kin is
  parsed first;
* always loaded: what draws a fixed monster (`MonsterMummy` in `src/world/dungeon_render_effects.cpp`);
* monsters already met: the journal bestiary draws them (`src/ui/journal_view.cpp`), and so does the death event
  (`src/state/game_state.cpp`, `event.who`). A save loaded mid-campaign, `--level N` and `gen:SEED:D` levels skip
  the earlier levels: load from the journal's met list.

## Design

* `Assets` gets `EnsureMonsters(set)`: loads the types in the set that are not loaded yet, keeps the rest.
  `monsterTypes[id]` (the stats from `MonsterKind`) stays filled for all types at start; only `model` and the
  `CharacterModel` wait.
* At level start (campaign, `--level`, load game, generated), compute the set above and call it. When something is
  missing, show the loading bar (`GameState::DrawLoad`).
* Fallback: drawing or spawning a monster whose model is not loaded loads it right then (a short hitch) and logs a
  warning naming the type and the level. No crash.
* The start load (`Assets::Load`) no longer loads monsters, so the loading bar spans change.
* The editor and `model-viewer`: check whether they use `Assets`, and load all types there.

## Tests

* Unit: the needed set for each campaign level (`content/levels/lvl*`) covers its spawns, their summons and kin.
  A level with a summoning boss includes the summons.
* Unit or levelcheck: no campaign level needs a type that has no model file.
* Scenario: play into a boss fight with summons, and load a save from a later level. Expect no fallback warning
  in the log.
* Timing before and after, written into this plan (commands in faster-loading).

## Open points

* Load everything at start in debug or test runs, as an option? That would catch nothing; the fallback warning
  is the guard.
* Load the next level's set in the background while the player plays? Only if the level change hitch is noticeable.
