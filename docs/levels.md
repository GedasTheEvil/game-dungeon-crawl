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
swimmer, 2.5 times its land speed; `Wading` in `src/entities/monster.h`, `WADING_DEFS` in `src/state/assets.cpp`). An arrow hits a monster standing in water for half its damage. Traps
work in it and show through the surface. `levelcheck` warns about half water that is not on deep water or a wall,
deep water not under water, a ladder that goes down into water (one may start in it) and a crocodile not in or next
to water.

Level files are text, version 2: a `DCLEVEL 2 40 47` header, the structure as a drawing, then the list of objects
(format: [tools/editor/readme.md](../tools/editor/readme.md#file-format)). Save games hold the same block for the
current level. Old (v1) level files and saves still load, converted as they are read; `levelcheck` warns about a v1
file, and `./levelconvert FILE...` (built by `make`) rewrites one as v2. Water: [crocodiles-and-flooded-cells.md](plan/crocodiles-and-flooded-cells.md).

## Campaign order

`src/world/campaign.h`. A game plays `levels/lvl1` to `levels/lvl15` (`CAMPAIGN_LEVELS`). Each exit loads the
next level. `levels/lvl15` holds the ankh that wins the game.

| Levels | Content |
|---|---|
| 1 | The way in: rats and a scarab, spikes, a spike pit to jump, ladders, the first chests. |
| 2 | The scarab galleries: scarabs, bats, plants by the chests, a riddle room up a ladder, a drop shaft, two spike pits. |
| 3 | The worm tunnels: worms, rats, a bat, a rock fall, the first key (red) and gate. A riddle gate in the exit hall. |
| 4 | The treasury: rats, scarabs, a worm, plants guarding side rooms, the first mimic among real chests. A riddle gate. |
| 5 | The scarab king, the first boss: a teleporter to the sealed boss room, the boss scarab and its scarabs. Behind the boss gate the blue key for the blue gate before the exit. A riddle room past a spike pit. |
| 6 | Rats, a giant rat, the first giant scarab. Red key and gate. |
| 7 | Bats and giant bats in low tunnels. Blue lever and gate. The first water: a flooded stretch of the west hall, the first crocodile in it, on the way to a chest. |
| 8 | Plants, worms, giant rats, giant scarabs. Red key, then the green key behind the red gate. The hall to the red gate is flooded, a crocodile in it. |
| 9 | Giant bats. Red key, red gate, then the blue lever behind it for the blue gate. Past the pit in the lower gallery a flooded stretch with a crocodile. |
| 10 | The vampire's roost, the second boss: the teleporter at the east end of the upper hall leads to the sealed roost, the vampire bat and its bats. Behind the boss gate the gold key for the gold gate to the shaft down. A giant scarab by the exit. |
| 11 | Giant rats, giant bats, giant scarabs, the first two mummies. Two levers (red, blue) in two halls open two gates in a row. |
| 12 | Plants, giant bats, three mummies. Chain: red key, green lever, gold key. |
| 13 | Rock falls, giant scarabs, giant rats, a mummy, the first Anubis. Blue key, gold lever. |
| 14 | Giant scarabs, rats and bats, two mummies, two Anubis. Red key, blue lever, green key. |
| 15 | The finale: three Anubis, two mummies, all four locks, riddles. The teleporter at the east end of the bottom hall leads to the sealed ankh chamber, the last boss: the Anubis boss and the mummies that climb out of the coffins round him. Behind the boss gate the ankh. |

Crocodiles live in the water of levels 7-9. Mummies appear from level 11 on, Anubis from level 13. Weak monsters give way to their giant kin: no rats, scarabs or small bats
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
| warnings | More than one entrance, keys, levers or treasure out of reach, gates that nothing opens, a teleporter pair id without exactly two teleporters, a boss gate without a boss or a boss without one, more than one boss, a boss the player can walk to (not only by teleporter), softlock cells, a path that must cross a death trap, an object in a wall or deep water, a level file still in v1. |
| size | Open cells, reachable cells, the bounding box. |
| content | Monsters by type, traps, treasure, keys, gates, levers, riddles. |
| path | The cheapest route: moves, jumps, drops, ladder steps, and the hazards, gates and monsters on it. |
| difficulty | The score used for ranking. About 5 is easy, 12 is medium, 20 or more is hard. |

A softlock cell is a cell the player can reach but cannot leave for the exit, for example the bottom of a
one-way drop. The bottom of a spike pit counts as a death, not as a softlock.

Difficulty score (`difficultyScore` in `level_check.cpp`): 0.04 per path move, 1 per spike, 4 per death trap,
1.5 per rock fall and 1.2 per jump on the path, 0.8 per gate, and 1 more for a jump over a death pit.
Monsters add their threat (`monsterThreat`: rat 0.7, scarab 0.8, bat 1.2, plant 1.5, worm 2, giant rat 3,
giant bat 3.5, giant scarab 4, crocodile 4.5, mimic 2, mummy 5, Anubis 8, boss scarab 10, vampire bat 12, anubis boss 15):
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
   At most 4 + 2 × difficulty monsters.
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
