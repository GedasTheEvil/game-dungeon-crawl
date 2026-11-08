# Same hazards for all: every trap and terrain hits the player and the monsters by one rule

Status: solved 2026-11-08 (play-tested by the user). Was: implemented 2026-11-08, waiting for the user's play-test. Was: draft 2026-11-06, size L, needs first: [shared-vitals](shared-vitals.tooling.md), [terrain-affinity](terrain-affinity.feature.md).

## Idea

From the user: the player is, in a way, one more monster that walks in and kills everything. So a trap, a poison, a
fire or a terrain treats every living thing by the same rule; a kind differs only by its numbers (resistances, trap
share, poison ward, native and hostile terrains), never by a separate code path. What stays the player's alone:
equipment, shrines and blessings, curses, skills, XP. With [shared-vitals](shared-vitals.tooling.md)
done, each point below is a change in one place.

## Plan

* **One trap formula.** Today the player's walk through spikes or a death trap is a bell curve over the hitbox, a
  share of max HP with a floor, faster at a sprint (`Dungeon::updateTrapCrossing`, `TrapBell`), while a monster takes
  `TrapHurt`'s ramp times `trapDamagePct` (`Monster::StandInTrap`). Use one for both: the crossing damage
  `max(floor, percent of max HP)` (`TrapBell::crossHundredths`) times the bell share times the speed factor, then
  times the body's trap share (the monster's `trapDamagePct`, the player's trap ward). Standing still in it (no way
  covered) keeps `TrapHurt`'s ramp for both. The same for ceiling spikes (`ceilingSpikesHit`, `Monster::CeilingSpikes`).
  Big monsters with a lot of HP take more than today: re-tune `trapDamagePct` per kind so a reckless crossing costs
  about what it costs now, unless the play-test says the new numbers are better.
* **Every hazard hits every body** (one function per hazard that walks the player and the monsters, not a player
  branch and a monster branch):
  * Blaze (Babi's burning floor, `Dungeon::updateBlazes`): today the player only; a monster in it takes
    `BLAZE_DAMAGE` and burns too (Babi himself: native fire, see below, or immune by resistance; the implementer's
    choice). A trap's kill: no XP.
  * Fire jet, poison drip, darts, falling rocks, sand falls: already both; route them through the one hit path with the
    same numbers. Where the monster's number differs from the player's today (`FIRE_JET_MONSTER_HP_PER_SECOND` beside
    the player's), keep one constant and let the trap share do the difference.
  * Webs: today a floor web slows the player (`WEB_SLOW_FACTOR`) and is a terrain for monsters. Make the web the
    terrain for the player too (below), with `WEB_SLOW_FACTOR` as the player's hostile speed in it.
  * Head bump: one rule (already near: `Player::HeadBump`, `Monster::HeadBump`), the daze / stun by the same roll.
* **Terrain for the player** (needs [terrain-affinity](terrain-affinity.feature.md)): the player has a
  terrain affinity like a monster kind: by default hostile to web (the slow above) and nothing else, so today's game
  plays the same. The hook is what matters: a blessing or a curse can later add a native or a hostile terrain (e.g. a
  god's favour makes water native: faster, harder hits there; a curse makes light hostile). Only the hook and the web
  row in this plan; new blessings and curses that use it are separate drafts. The HUD shows the player's terrain mood
  as a status card (Condition grey for hostile, Blessing gold for native, `statusList`).
* **Conditions for all**: a monster can be dazed by what dazes the player (a screech, a storm roar) where a monster can
  be hit by it at all; today they only hit the player, so this is only the shared timer, no new hit. Blind stays the
  player's (a monster has no view to blur).
* **The level checker** (`level_check*.cpp`, `level_walker`) models trap damage to the player and to reckless monsters
  ([checker-reckless-threat](checker-reckless-threat.tooling.md)): switch it to the one formula, then fix every
  campaign level it flags (AGENTS.md: no warnings).
* **Journal**: a creature's note on traps ("walks through spikes unharmed") reads its trap share as before.

## Out of scope

* Shrines and blessings for monsters, equipment for monsters.
* Balance of the bosses (separate follow-ups after the play-test).
* The icons over the monsters: [monster-status-icons](monster-status-icons.feature.md).

## Open points (implementer's choice)

* Exact `trapDamagePct` re-tune per kind after the formula switch (start: keep a reckless crossing within ±25% of
  today's damage; a unit test per reckless kind pins it).
* Babi in his own blaze: immune by fire resistance, or native fire from terrain-affinity.
* Whether the player's terrain card shows always or only when the mood is not neutral (likely only then).

## Done

* **One trap formula** (`Dungeon::TrapWalkHundredths`, `TrapBell::walkHundredths`): every body on foot in spikes or a
  death trap takes the bell over the way it covered at its pace (the player's measured, a monster's walk) times
  `max(floor, percent of max HP)`, plus `TrapHurt`'s ramp, times its trap share. The share is per-mille now
  (`MonsterKind::trapPermille`, `PlayerStats::TrapPermille`, `Vitals::TrapLoss`): the old percent could not hold a
  boss's share (below 1%). Traps ignore armour for every body (the player's ramp hits too). The death trap's venom:
  once per entry for both (`TrapVenomDue`). Ceiling spikes: a whole walk at the walk pace for both.
* **Re-tune** (per-mille): Anubis guard 19, mummy 440, Anubis boss 4, scorpion queen 72, Apep 71, Sobek 84, mummy
  priest 75, Seth 78, Rerek 78, Babi 11: each reckless kind's walk through spikes within ±25% of before
  (`same_hazards_test.cpp` pins it). Cowards keep 1000.
* **Side effects for the play-test** (one share for all traps): darts, rocks, sand falls, drips and wall stabs keep
  their constants but use the new share, so the Anubis guard and the bosses barely feel them; the fire jet has one
  rule (`fireJetHundredthsPerSecond`, `FIRE_JET_MONSTER_HP_PER_SECOND` gone): a mummy (weak to fire) now melts in
  a jet in about a second, a coward caught in one dies; a giant rat pouncing into ceiling spikes dies (the player's
  floor of 200 HP, as a new player).
* **Blaze**: hits every body on the ground by one timer (`Vitals::blazeReadyMs`); a monster native to fire (Babi, his
  baboons) is unharmed; no XP for its kill.
* **Webs / terrain for the player**: `PlayerStats::TerrainNative` / `TerrainHostile` (the hook; today webs hostile),
  `PLAYER_TERRAIN_ROWS` (a web only slows the player to `WEB_SLOW_FACTOR`), `Dungeon::PlayerTerrainFactors` drives the
  walk; the HUD shows the mood card only when not neutral.
* **Head bump and conditions**: already one rule (`rollHeadBumpDaze`, `Vitals::KnockOut`); no new hits.
* **Checker**: it models no trap damage numbers (a weight per trap on the path); the reckless reach is
  [checker-reckless-threat](checker-reckless-threat.tooling.md). All campaign levels pass with no warnings.
* **Journal**: no creature note reads the trap share; nothing to change.

## Play-test

* `bin/game --play tests/play/same-hazards-for-all.txt`: wake the mummy behind the spikes; it walks into them and
  dies there, as before. Cross the spikes yourself: as deadly as before.
* In a Mother of Webs fight, stand in a web: slowed as before, a grey terrain card shows on the HUD.
* In Babi's fight: his burning floor hurts you as before, not him or his baboons.
* Watch for the side effects above (jets, darts on the Anubis guard): say if the new numbers feel wrong.
