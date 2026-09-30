# Levels: validator, generator, campaign

Level files, tile types and the editor: [tools/editor/readme.md](../tools/editor/readme.md).
Tile types in code: `src/world/level.h`.

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
| 7 | Bats and giant bats in low tunnels. Blue lever and gate. |
| 8 | Plants, worms, giant rats, giant scarabs. Red key, then the green key behind the red gate. |
| 9 | Giant bats. Red key, red gate, then the blue lever behind it for the blue gate. |
| 10 | The first Anubis, by the exit. Gold key and gate. |
| 11 | Giant rats, giant bats, giant scarabs, an Anubis. Two levers (red, blue) in two halls open two gates in a row. |
| 12 | Plants, giant bats, two Anubis. Chain: red key, green lever, gold key. |
| 13 | Rock falls, giant scarabs, giant rats, an Anubis. Blue key, gold lever. |
| 14 | Giant scarabs, rats and bats, two Anubis. Red key, blue lever, green key. |
| 15 | The finale: three Anubis, all four locks, riddles, the ankh. |

Anubis only appears from level 10 on. Weak monsters give way to their giant kin: no rats, scarabs or small bats
after level 9, no small scarabs after level 5 (giant scarabs from 6). The sources of all levels are ASCII drawings in `tools/level/campaign/`
(see [Test levels from ASCII](#test-levels-from-ascii)). Rebuild one with
`python3 tools/level/ascii2level.py tools/level/campaign/lvl9.txt levels/lvl9`.

## levelcheck: validate and rank

```
make level-tools
./levelcheck levels/lvl*                   # report per level, then a ranking, easiest first
./levelcheck --map levels/lvl3             # plus the map with the path drawn as '*'
./levelcheck --script tests/out/paths levels/lvl*   # a scenario per level that plays the path
```

Exit code: 0 if all levels are valid, 1 if one cannot be finished, 2 for a usage error or an unreadable file.

The check walks the level with the player's movement rules from `Dungeon` (`src/world/level_check.h`):

- Walk left or right into any open cell. With no floor below, fall straight down to a floor or a ladder.
- Climb between vertically adjacent `Ladder` cells.
- Jump over a gap of one cell. The jump is about 0.45 tiles high, so the player cannot step up onto a ledge.
- A key, or a pulled lever, opens every gate of its colour.
- Reaching the boss counts as killing it: the boss gates (lock colour 5) open.
- A teleporter (Door, gate type 5) jumps to the other teleporter with the same pair id, both ways.

It reports:

| Item | Meaning |
|---|---|
| errors | No entrance, no exit or ankh, or the exit cannot be reached. The level is invalid. |
| warnings | More than one entrance, keys, levers or treasure out of reach, gates that nothing opens, a teleporter pair id without exactly two teleporters, a boss gate without a boss or a boss without one, more than one boss, a boss the player can walk to (not only by teleporter), softlock cells, a path that must cross a death trap. |
| size | Open cells, reachable cells, the bounding box. |
| content | Monsters by type, traps, treasure, keys, gates, levers, riddles. |
| path | The cheapest route: moves, jumps, drops, ladder steps, and the hazards, gates and monsters on it. |
| difficulty | The score used for ranking. About 5 is easy, 12 is medium, 20 or more is hard. |

A softlock cell is a cell the player can reach but cannot leave for the exit, for example the bottom of a
one-way drop. The bottom of a spike pit counts as a death, not as a softlock.

Difficulty score (`difficultyScore` in `level_check.cpp`): 0.04 per path move, 1 per spike, 4 per death trap,
1.5 per rock fall and 1.2 per jump on the path, 0.8 per gate, and 1 more for a jump over a death pit.
Monsters add their threat (`monsterThreat`: rat 0.7, scarab 0.8, bat 1.2, plant 1.5, worm 2, giant rat 3,
giant bat 3.5, giant scarab 4, mimic 2, Anubis 8, boss scarab 10):
the full value within 3 cells of the path, a quarter elsewhere. Each reachable treasure takes 0.2 off.

The ranking keeps the finale (a level with the ankh) last.

### Playing the path in the game

`--script DIR` writes `DIR/<level>.txt`, a [scenario](testing.md) that plays the path in god mode and expects the
level to be finished (`expect level == 2`, or `expect won == 1` for the finale). Run it with
`make test SCENARIO=DIR/<level>.txt`. It checks the movement model against the real physics.
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
   worm 3-7, plant from 3, giant rat from 4, giant scarab from 5, giant bat from 6, Anubis from 8.
   At most 4 + 2 × difficulty monsters.
   From difficulty 2, 15% of the treasure chests are mimics, drawn from a separate random stream. Most seeds
   keep their layout; where a mimic changes the difficulty score, the generator may pick another candidate.
   A mimic is `M` in the legend; the draft map shows it as a chest. A giant scarab is `k`, the boss scarab `K`, a boss gate `Z`, a teleporter `O`.

Each candidate goes through `checkLevel`. The generator only keeps a level that is valid and has no warnings
(so no softlocks, and every key, lever and treasure is in reach). Of up to 80 candidates it returns the one whose
score is closest to `targetScore(difficulty)` = 2.5 + 2.2 × difficulty, and stops early within 10 %.

Scenario tests can load a generated level directly: `level gen:SEED:DIFFICULTY` ([testing.md](testing.md)).

## Test levels from ASCII

`tools/level/ascii2level.py IN.txt OUT` builds a level file from a drawing in the `levelcheck --map` legend.
A `def CHAR TYPE ATTR VALUE` line adds a character to the legend for that file, for example `def L 12 2 0`
(blue lever) or `def 1 8 3 1` (large health potion).
Examples: `tests/levels/mechanisms.txt`, `tests/levels/rats.txt`, `tests/levels/bats.txt` (the built files sit next to them).

## Tests

- `tests/scenarios/mechanisms.txt`: key, gates, lever, rock falls (hit and dodged), keys kept over save and load.
- `tests/scenarios/rock_fall.txt`: rock fall walked through, graze, stepped back from, direct hit.
- `tests/scenarios/rats.txt`: rat and giant rat screenshots (size, attack, die).
- `tests/scenarios/monster_hazards.txt`: walkers stop at floor spikes and a pit; only flyers cross them.
- `tests/scenarios/giant_rat_jump.txt`: the giant rat leaps spikes and a pit (2 s apart), not a 3-cell gap.
- `tests/scenarios/giant_scarab_jump.txt`: the giant scarab leaps spikes and a pit and bites.
- `tests/scenarios/scarabs.txt`: scarab and giant scarab screenshots (size, texture).
- `tests/scenarios/mimic.txt`: a mimic next to a real chest: looks like the chest, wakes 1.5 tiles away, bites, leaves
  a real chest when killed.
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
- `tests/levels/classic1`, `classic2`: the old hand-made levels 1 and 2, kept as fixtures for the scenarios that
  depend on their layout (ladders, chests, the draft map, ...), so the campaign levels can change.
- `tests/scenarios/generated.txt`: a generated level loads (`gen:SEED:D`), campaign levels 6 and 15 load.
- `tests/scenarios/generated_path.txt`: plays `tests/levels/gen_d8` (seed 81, difficulty 8) from entrance to exit.
  Written by `levelcheck --script`.
