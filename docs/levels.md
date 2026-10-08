# Levels: validator, generator, campaign

Level files, tile types and the editor: [tools/editor/readme.md](../tools/editor/readme.md).
Tile types and structures in code: `src/world/level.h`.

## Level format

A cell has two layers ([level-format-layers.md](plan/solved/level-format-layers.md)):

* **Structure**: `Wall`, `Empty`, `HalfWater`, `DeepWater`. A cell blocks when it is `Wall` or `DeepWater`.
* **Object**: at most one thing in the cell, with attribute and value: ladder, door, gate, lever, key, treasure,
  ankh, monster spawn, spike, death trap, rock fall. An object stands in `Empty` or `HalfWater`.

Water rules (the game, `levelcheck` as far as it models them): the player wades through half water at half speed and
cannot sprint or jump while standing in it (jumping into it is fine, so a pool can be a trap); a fall into it lands like on a
floor. Walkers wade too: slowed (scarabs, the worm, the mummy), unaffected (rats, Anubis), or faster (the crocodile, a
swimmer, 2.5 times its land speed; `Wading` and `KINDS` in `src/world/monster_kinds.*`). An arrow hits a monster standing in water for half its damage. Traps
work in it and show through the surface. `levelcheck` warns about half water that is not on deep water or a wall,
deep water not under water, a ladder that goes down into water (one may start in it) and a crocodile not in or next
to water.

Level files are text, version 2: a `DCLEVEL 2 40 47` header, the structure as a drawing, then the list of objects
(format: [tools/editor/readme.md](../tools/editor/readme.md#file-format)). Save games hold the same block for the
current level. Old (v1) level files and saves still load, converted as they are read; `levelcheck` warns about a v1
file, and `./levelconvert FILE...` (built by `make`) rewrites one as v2. Water: [crocodiles-and-flooded-cells.md](plan/solved/crocodiles-and-flooded-cells.md).

## Campaign order

`src/world/campaign.h`. A game plays `levels/lvl1` to `levels/lvl30` (`CAMPAIGN_LEVELS`), a boss every five
levels ([longer-campaign.md](plan/solved/longer-campaign.md)). Each exit loads the next level. `levels/lvl30` holds the ankh that wins the game; the Anubis boss there is always the last boss.
Levels 15-30 use the whole 40 x 47 grid ([denser-levels.draft.md](plan/denser-levels.draft.md)).

| Levels | Content |
|---|---|
| 1 | The way in: rats and a scarab, spikes, a spike pit to jump, ladders, the first chests. |
| 2 | The scarab galleries: scarabs, bats, plants by the chests, a riddle room up a ladder, a drop shaft, two spike pits. |
| 3 | The worm tunnels: worms, rats, a bat, the first scorpion, a rock fall, the first key (red) and gate. A riddle gate in the exit hall. An antidote in a niche past the shaft at the east end of the upper hall. |
| 4 | The treasury: rats, scarabs, a scorpion, a worm, plants guarding side rooms, the first mimic among real chests. A riddle gate. An antidote at the east end of the lower hall. |
| 5 | The scarab king, the first boss: a teleporter to the sealed boss room, the boss scarab and its scarabs. Behind the boss gate the blue key for the blue gate before the exit. A riddle room past a spike pit. A scorpion in the exit hall, an antidote beside the ladder down to it. |
| 6 | Rats, a giant rat, the first giant scarab, two scorpions. Red key and gate. An antidote where the shaft lands in the lower den. |
| 7 | Bats and giant bats in low tunnels. Blue lever and gate. The first water: a flooded stretch of the west hall, the first crocodile in it, on the way to a chest. |
| 8 | Plants, worms, giant rats, giant scarabs. Red key, then the green key behind the red gate. The hall to the red gate is flooded, a crocodile in it. |
| 9 | Giant bats. Red key, red gate, then the blue lever behind it for the blue gate. Past the pit in the lower gallery a flooded stretch with a crocodile. |
| 10 | The vampire's roost, the second boss: the teleporter at the east end of the upper hall leads to the sealed roost, the vampire bat and its bats. Behind the boss gate the gold key for the gold gate to the shaft down. A giant scarab by the exit. |
| 11 | Giant rats, giant bats, giant scarabs, the first two mummies. Two levers (red, blue) in two halls open two gates in a row. |
| 12 | Plants, giant bats, three mummies. Chain: red key, green lever, gold key. |
| 13 | Rock falls, giant scarabs, giant rats, a mummy, the first Anubis. Blue key, gold lever. |
| 14 | Giant scarabs, rats and bats, two mummies, two Anubis. Red key, blue lever, green key. |
| 15 | The scorpion queen's nest, the third boss: giant scorpions and scorpions in every hall, mummies, giant scarabs, giant bats, an Anubis. The blue lever opens the way down to the nest, a cavern with two ledges (green key, green gate). The teleporter leads to the sealed queen's chamber: the scorpion queen between two egg clusters her brood hatches from; behind the boss gate the gold key for the gold gate to the spike pits and the exit cavern. Antidotes in the second hall and past the pits. |
| 16 | The serpent temple's gate: cobras, crocodiles, mummies, two Anubis, giant bats. A loop of two halls round a pillar (red key, red gate, a riddle chamber), a flooded hall with a deep crocodile pool (blue lever), a rock-fall gauntlet with a drop / ladder loop to the green key; the gold lever in a side gallery opens the exit hall's gold gate. The first dart trap, in the upper loop hall, a mummy east of it. |
| 17 | The cistern: the entrance hall forks into two wings, the west one to the red key and the blue lever (an Anubis), the east one behind the red gate down into the flooded cistern (crocodiles, a swimming cobra, the green key on a ledge). The rock-fall drain, the pool hall, the gold lever, death pits before the exit. A riddle shrine and a snake pit between the wings. |
| 18 | The sanctuary of the uraeus, climbed bottom to top: the hall of two gates (red lever below, blue lever above), a pillar loop with the green key inside the pillar, a rock-fall gauntlet, a 3-high flooded hall with crocodiles and a riddle gate, the gold key by an Anubis. |
| 19 | The serpent sanctum: cobras, giant bats, mummies, five Anubis, two crocodiles in a two-high flooded hall. Red key, blue lever, green key past a drop shaft, gold lever beyond a loop round a pillar, then a spike-pit gauntlet before the exit hall. |
| 20 | Apep's pit, the fourth boss: cobras, mummies, Anubis, Apep's throat (a shaft from the second hall down to the fifth), a crocodile pool. Red key, blue lever, green lever; the teleporter in the ninth hall's east nook leads to the sealed pit: a sunken floor between two ledges, Apep and his cobras, the boss gate on the east ledge, behind it the gold key for the gold gate before the exit. |
| 21 | The flooded halls: giant cobras, seven crocodiles in long wades, giant bats, mummies, Anubis. Red lever into the two-high flooded hall, blue key, green key past the riddle, gold lever by the last crocodile; a teleporter to a hidden treasure cistern. |
| 22 | The cistern grid: halls split by a central wall and joined by full-width passages; giant cobras, crocodile pools, mummies, Anubis, giant bats. Red key, blue lever, green lever down a dead end, gold key behind a mummy; a rock-fall gauntlet, spike pits before the exit hall. |
| 23 | The cistern stairs: fifteen halls down in a zigzag, five of them flooded; crocodiles, giant cobras, giant bats, mummies, Anubis. Red key, blue lever, green key, gold lever. A dart trap in the blue lever's hall, an Anubis coming over it. |
| 24 | The twin cisterns: down the west wing, across a flooded hall, through a sunken crypt, up the east wing to the exit. Gold lever, red key, blue lever in the crypt; the green key near the top opens the exit gate. A bridge over a well joins the wings. |
| 25 | Sobek's lake, the fifth boss: crocodiles in every pool, giant cobras, Anubis guards, a loop round a pillar with a red gate on each side. The blue lever opens the gate to the teleporter: the sealed flooded lake with two rock shelves, Sobek and his crocodiles; behind the boss gate the gold key for the gold gate on the way to the exit. |
| 26 | The necropolis: Anubis guards, mummies in a coffin row, giant cobras, giant scarabs, giant scorpions, loose ceilings everywhere. A processional stair as a hub with wings: red key, red gate, blue lever, blue gate, green key, green gate; the gold lever before the exit. |
| 27 | The necropolis gate: a loop of two halls round a pillar with a hidden chamber inside, a flooded crypt with giant cobras, a rock-fall gauntlet and a riddle room. Red key, blue lever, green key, gold lever (two gold gates: the way down and a treasure tomb). |
| 28 | The catacombs: the hall of coffins (3 high, two ledges, the red lever), the blue key, a flooded tomb, the green lever (two green gates), the gold key on a ledge in the 3-high hall of Anubis. |
| 29 | The two towers of the dead: the red key opens the bridge to the east tower, the blue lever there the way back west into the great flooded hall (giant cobras in the water, the green key on an island); the gold key, the Anubis hall, a rock-fall gauntlet to the exit, a drop to a hidden tomb. |
| 30 | The ankh chamber, the finale: four lock halls (red key, blue lever, green key past a flooded stretch, gold lever), a hall of coffins and two rock-fall gauntlets down to the teleporter. It leads to the sealed 3-high ankh chamber: the Anubis boss among his coffins, his mummies climbing out of them; behind the boss gate the treasure and the ankh that wins the game. A long ladder past the chamber wall to a bottom treasure gallery. |

Scorpions live in levels 3-6, giant scorpions in 15 and 26-30, cobras in 16-20, giant cobras in 21-30; each level with a poisoner has an antidote in reach (the checker warns otherwise). Crocodiles live in the water of levels 7-9. Mummies appear from level 11 on, Anubis from level 13. Weak monsters give way to their giant kin: no rats, scarabs or small bats
after level 9, no small scarabs after level 5 (giant scarabs from 6), except a boss's minions (the vampire bat's bats in 10). The sources of all levels are ASCII drawings in `tools/level/campaign/`
(see [Test levels from ASCII](#test-levels-from-ascii)). Rebuild one with
`python3 tools/level/ascii2level.py tools/level/campaign/lvl9.txt levels/lvl9`.

## levelcheck: validate and rank

```
make level-tools
./levelcheck levels/lvl*                   # report per level, then a ranking, easiest first
./levelcheck --map levels/lvl3             # plus the map with the path drawn as '*'
./levelcheck --legend                      # the map's characters, with the tile each one stands for
./levelconvert levels/lvl*                 # rewrite v1 level files as v2 (v2 files are left alone)
./levelcheck --script tests/out/paths levels/lvl*   # a scenario per level that plays the path
```

Exit code: 0 if all levels are valid, 1 if one cannot be finished, 2 for a usage error or an unreadable file.

The check walks the level with the player's movement rules from `Dungeon` (`src/world/level_check.h`):

- Walk left or right into any open cell. With no floor below, fall straight down to a floor or a ladder.
- Climb between vertically adjacent `Ladder` cells.
- Jump over one cell: a gap in the floor, or spikes or a death trap on it, from a floor (a ladder's foot too). The
  jump peaks 0.4 tiles high (`Jump::ARC`, `src/world/movement.h`, from the game's jump constants), so the player
  cannot step up onto a ledge. A rock fall is walked, not jumped (it drops all the same). Jumps from mid-ladder are
  not modelled. Stamina is not counted: it comes back while the player waits.
- A key, or pulling a lever, opens every gate of its colour. In the level file a gate is closed only with value 0
  (the game opens one saved while opening), and a lever with value 1 is already pulled and opens nothing.
- Reaching the boss counts as killing it (the checker cannot fight): the boss gates (lock colour 5) open. No key
  opens them.
- A teleporter (Door, gate type 5) jumps to the other teleporter with the same pair id, both ways.

It reports:

| Item | Meaning |
|---|---|
| errors | No entrance, no exit or ankh, or the exit cannot be reached. The level is invalid. |
| warnings | More than one entrance, keys, levers or treasure out of reach, gates that nothing opens, a teleporter pair id without exactly two teleporters, a boss gate without a boss or a boss without one, more than one boss, a boss the player can walk to (not only by teleporter), softlock cells, a path that must cross a death trap, an object in a wall or deep water, a level file still in v1, a poisoner (scorpion, cobra) without an antidote in reach. |
| size | Open cells, reachable cells, the bounding box. |
| content | Monsters by type, traps, treasure, keys, gates, levers, riddles. |
| path | The cheapest route: moves, jumps, drops, ladder steps, and the hazards, gates and monsters on it. |
| difficulty | The score used for ranking. About 5 is easy, 12 is medium, 20 or more is hard. |

A softlock cell is a cell the player can reach but cannot leave for the exit, for example the bottom of a
one-way drop. The bottom of a spike pit counts as a death, not as a softlock.

Difficulty score (`difficultyScore` in `level_check.cpp`): 0.04 per path move, 1 per spike, 4 per death trap,
1.5 per rock fall and 1.2 per jump on the path, 0.8 per gate, and 1 more for a jump over a death pit.
Monsters add their threat (`monsterThreat`: rat 0.7, scarab 0.8, bat 1.2, plant 1.5, worm 2, giant rat 3,
giant bat 3.5, giant scarab 4, crocodile 4.5, mimic 2, mummy 5, Anubis 8, boss scarab 10, vampire bat 12, anubis boss 15, scorpion 1, cobra 3, giant cobra 5, giant scorpion 3.5, egg cluster 0.5, scorpion queen 13, Apep 14, Sobek 14.5):
the full value within 3 cells of the path, a quarter elsewhere. Each reachable treasure takes 0.2 off.

The ranking keeps the finale (a level with the ankh) last.

### Playing the path in the game

`--script DIR` writes `DIR/<level>.txt`, a [scenario](testing.md) that plays the path in god mode and expects the
level to be finished (`expect level == 2`, or `expect won == 1` for the finale). Run it with
`make test SCENARIO=DIR/<level>.txt`; `make paths` does it for every campaign level. It checks the movement model
against the real physics.
Monsters and damage do not stop it, so it proves that the level can be crossed, not that it is fair.

## levelgen: random levels

```
./levelgen --seed 7 --difficulty 5 --map levels/test1    # one level, print the map
./levelgen --seed 1 --difficulty 3 --count 5 /tmp/gen    # /tmp/gen1 .. /tmp/gen5, seeds 1 .. 5
```

Difficulty runs from 1 to 10. The same seed and difficulty always give the same level.

The generator (`src/world/level_gen.cpp`) builds levels like the hand-made ones:

1. **Main route.** 3 + difficulty / 2 corridors, one cell high (sometimes a two-cell hall), 6 to 15 cells long.
   Ladders (both ways) and drop shafts (one way down) join them. The entrance is at one end of the first
   corridor, the exit at the far end of the last one.
2. **Locks.** From difficulty 3 there is one gate, from 6 two, from 9 three. A branch on the near side of the
   gate holds its key, or in about a third of the cases a lever.
3. **Branches.** Short dead-end side corridors off the route, joined by a ladder, with treasure at the end.
4. **Content.** Along the part of each corridor the player has to cross: spike pits to jump, spikes, rock
   falls (only under a one-cell ceiling), monsters and treasure. The mix grows with the difficulty. Monster types
   unlock with the difficulty and the weak ones give way to their giant kin: rat and scarab 1-5, bat 2-6,
   worm 3-7, plant from 3, giant rat from 4, giant scarab from 5, giant bat from 6, mummy from 7, Anubis from 9.
   At most 4 + 2 × difficulty monsters. Weapon chests come in by depth like in the campaign: club and dagger, the
   short sword and spear from difficulty 3, the khopesh from 5, the axes and the mace from 7; self-bow and sling, the
   throwing stick and javelin from 4, the composite bow from 7.
   From difficulty 2, 15% of the treasure chests are mimics, drawn from a separate random stream. Most seeds
   keep their layout; where a mimic changes the difficulty score, the generator may pick another candidate.
   A mimic is `M` in the legend; the draft map shows it as a chest. A giant scarab is `k`, the boss scarab `K`, the vampire bat `V`, the Anubis boss `N`, a mummy `u`, a boss gate `Z`, a teleporter `O`.

Each candidate goes through `checkLevel`. The generator only keeps a level that is valid and has no warnings
(so no softlocks, and every key, lever and treasure is in reach). Of up to 80 candidates it returns the one whose
score is closest to `targetScore(difficulty)` = 2.5 + 2.2 × difficulty, and stops early within 10 %.

Scenario tests can load a generated level directly: `level gen:SEED:DIFFICULTY` ([testing.md](testing.md)).

## Test levels from ASCII

`tools/level/ascii2level.py IN.txt OUT` builds a level file from a drawing in the `levelcheck --map` legend. It
reads the legend from `levelcheck --legend`, which prints the game's tables (`src/world/tile_defs.cpp`,
`monster_kinds.cpp`, the lock colours in `level.h`), so the drawing, the checker and the game cannot disagree.
In `--map` output, `m`, `q` and `Q` mark a monster type, key colour or gate colour the game does not know.
A `def CHAR TYPE ATTR VALUE` line adds a character to the legend for that file, for example `def L 12 2 0`
(blue lever) or `def 1 8 3 1` (large health potion).
The drawing is the object layer. A line `structure` can follow it with a second drawing of the same rows, the
structure: `#` wall, `.` empty, `~` half water, `=` deep water. Then `#` and `.` in the first drawing just mean no
object. Without it, the structure follows from the object drawing: `#` wall, anything else empty. `levelcheck --map`
prints the structure drawing after the map when the level has water.
Examples: `tests/levels/mechanisms.txt`, `tests/levels/rats.txt`, `tests/levels/bats.txt` (the built files sit next to them).

## Tests

- `tests/scenarios/mechanisms.txt`: key, gates, lever, rock falls (hit and dodged), keys kept over save and load.
- `tests/scenarios/rock_fall.txt`: rock fall walked through, graze, stepped back from, direct hit.
- `tests/scenarios/dart_trap.txt`: a dart trap's volley outruns the walk and poisons, a running jump over the plate
  sets nothing off, a heavy monster (the Anubis) presses it and takes the darts.
- `tests/scenarios/rats.txt`: rat and giant rat screenshots (size, attack, die).
- `tests/scenarios/monster_hazards.txt`: walkers stop at floor spikes and a pit; only flyers cross them.
- `tests/scenarios/coward_rock.txt`: a rat stops at an armed rock fall, a giant rat leaps over it; neither sets it off.
- `tests/scenarios/reckless_spikes.txt`, `reckless_rock.txt`: a reckless monster (the mummy; the Anubis and the Anubis boss too,
  `reckless_anubis.txt`)
  walks into spikes and a rock fall and takes its share of the damage (`Courage`, `trapDamagePct`); a trap's kill
  gives no XP.
- `tests/scenarios/giant_rat_jump.txt`: the giant rat leaps spikes and a pit (2 s apart), not a 3-cell gap.
- `tests/scenarios/giant_scarab_jump.txt`: the giant scarab leaps spikes and a pit and bites.
- `tests/scenarios/spikes.txt`: spike damage rate and ramp, the hitbox edge, paused in the inventory, two tiles.
- `tests/scenarios/monster_hitboxes.txt`: scarab, rat, worm, giant scarab, boss scarab and giant rat walk up and stop
  where they bite; every melee weapon hits them there; an arrow hits a giant rat 2 tiles away (`tests/levels/hitbox_*`).
- `tests/scenarios/scarabs.txt`: scarab and giant scarab screenshots (size, texture).
- `tests/scenarios/mimic.txt`: a mimic next to a real chest: looks like the chest, wakes 1.5 tiles away, bites, leaves
  a real chest when killed.
- `tests/scenarios/mummy.txt`: a mummy in its coffin: no health bar, wakes 1.6 tiles away, climbs out (rise clip),
  strikes, dies; the empty coffin stays.
- `tests/scenarios/bats.txt`: bat and giant bat screenshots (roosting, swoops through the player, kill, fall).
- `tests/scenarios/bow.txt`: the bow draws and shoots; an arrow with nothing in reach lands on the floor, one in reach is
  aimed at the plant (`tests/levels/archery`) and hits it, the shots kill it.
- `tests/scenarios/ranged_weapons.txt`: composite bow, sling, throwing stick and javelin each hit the archery plant
  from inside their range; screenshots of the draw, the whirl and the missiles in flight (the stick flying back).
- `tests/scenarios/melee_weapons.txt`: dagger, khopesh, the axes and the mace each reach a giant rat at bite distance.
- `tests/scenarios/attack_recovery.txt`: sprinting through a recovery does not cut it short.
- `tests/scenarios/weapons_held.txt`: every weapon in the fist, standing and through its attack (windup, strike,
  recovery), both facings: screenshots.
- `tests/scenarios/monster_idle_bars.txt`: health bars stay hidden until a monster chases, bites, swoops or is hit.
- `tests/scenarios/teleport.txt`: a teleporter pair (`tests/levels/teleport`): the jump there and back, the exit in
  the room only the teleporter reaches.
- `tests/scenarios/boss.txt`: the boss room (`tests/levels/boss`): minions on arrival and summoned up to the limit,
  1 XP per minion while the boss lives, the sealed boss gate, the boss's death opens it, no boss after a load.
- `tests/scenarios/lvl5_boss.txt`: lvl5 played through in god mode: teleporter, boss fight, blue key, exit.
- `tests/scenarios/vampire.txt`: the vampire bat's roost (`tests/levels/vampire`): its bats, its bites heal it
  (`hurtboss`), more bats up to the limit, its death opens the boss gate to the gold key.
- `tests/scenarios/anubis_boss.txt`: the Anubis boss's chamber (`tests/levels/anubis_boss`): mummies climb out of
  the coffins round him, more up to the limit, his blows on a level 30 player, his death opens the boss gate.
- `tests/scenarios/summon_effects.txt`: scarabs digging out in a spray of sand, bats dropping from the ceiling,
  mummies out of the Anubis boss's coffins (`tests/levels/summon_dig`, `summon_drop`, `summon_coffin`): screenshots.
- `tests/levels/classic1`, `classic2`: the old hand-made levels 1 and 2, kept as fixtures for the scenarios that
  depend on their layout (ladders, chests, the draft map, ...), so the campaign levels can change.
- `tests/scenarios/generated.txt`: a generated level loads (`gen:SEED:D`), campaign levels 6 and 15 load.
- `tests/scenarios/generated_path.txt`: plays `tests/levels/gen_d8` (seed 81, difficulty 8) from entrance to exit.
  Written by `levelcheck --script`.
