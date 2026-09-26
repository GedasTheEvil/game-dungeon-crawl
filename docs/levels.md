# Levels: validator, generator, campaign

Level files, tile types and the editor: [DungeonEditor/readme.md](../DungeonEditor/readme.md).
Tile types in code: `src/world/level.h`.

## Campaign order

`src/world/campaign.h`. A game plays:

1. `Levels/lvl1` to `Levels/lvl4` (hand-made).
2. Six generated levels, difficulty 3 to 8.
3. `Levels/lvl5`, the ankh finale.

New Game picks a run seed from the clock. The save keeps it after the map, so a loaded game gets the same
generated levels. Saves from before the run seed load with seed 1.
The constants `CAMPAIGN_OPENING`, `CAMPAIGN_GENERATED` and `CAMPAIGN_FIRST_DIFFICULTY` set the mix.

## levelcheck: validate and rank

```
make level-tools
./levelcheck Levels/lvl*                   # report per level, then a ranking, easiest first
./levelcheck --map Levels/lvl3             # plus the map with the path drawn as '*'
./levelcheck --script tests/out/paths Levels/lvl*   # a scenario per level that plays the path
```

Exit code: 0 if all levels are valid, 1 if one cannot be finished, 2 for a usage error or an unreadable file.

The check walks the level with the player's movement rules from `Dungeon` (`src/world/level_check.h`):

- Walk left or right into any open cell. With no floor below, fall straight down to a floor or a ladder.
- Climb between vertically adjacent `Ladder` cells.
- Jump over a gap of one cell. The jump is about 0.45 tiles high, so the player cannot step up onto a ledge.
- A key, or a pulled lever, opens every gate of its colour.

It reports:

| Item | Meaning |
|---|---|
| errors | No entrance, no exit or ankh, or the exit cannot be reached. The level is invalid. |
| warnings | More than one entrance, keys, levers or treasure out of reach, gates that nothing opens, softlock cells, a path that must cross a death trap. |
| size | Open cells, reachable cells, the bounding box. |
| content | Monsters by type, traps, treasure, keys, gates, levers, riddles. |
| path | The cheapest route: moves, jumps, drops, ladder steps, and the hazards, gates and monsters on it. |
| difficulty | The score used for ranking. About 5 is easy, 12 is medium, 20 or more is hard. |

A softlock cell is a cell the player can reach but cannot leave for the exit, for example the bottom of a
one-way drop. The bottom of a spike pit counts as a death, not as a softlock.

Difficulty score (`difficultyScore` in `level_check.cpp`): 0.04 per path move, 1 per spike, 4 per death trap,
1.5 per rock fall and 1.2 per jump on the path, 0.8 per gate, and 1 more for a jump over a death pit.
Monsters add their threat (`monsterThreat`: rat 0.6, scarab 1, bat 1.2, plant 1.5, worm 2.5, giant rat 3,
giant bat 3.5, Anubis 8):
the full value within 3 cells of the path, a quarter elsewhere. Each reachable treasure takes 0.2 off.

The ranking keeps the finale (a level with the ankh) last.

### Playing the path in the game

`--script DIR` writes `DIR/<level>.txt`, a [scenario](testing.md) that plays the path in god mode and expects the
level to be finished (`expect level == 2`, or `expect won == 1` for the finale). Run it with
`make test SCENARIO=DIR/<level>.txt`. It checks the movement model against the real physics.
Monsters and damage do not stop it, so it proves that the level can be crossed, not that it is fair.

## levelgen: random levels

```
./levelgen --seed 7 --difficulty 5 --map Levels/test1    # one level, print the map
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
   unlock with the difficulty: rat and scarab from 1, bat from 2, plant and worm from 3, giant rat from 4,
   giant bat from 5, Anubis from 8.
   At most 4 + 2 × difficulty monsters.

Each candidate goes through `checkLevel`. The generator only keeps a level that is valid and has no warnings
(so no softlocks, and every key, lever and treasure is in reach). Of up to 80 candidates it returns the one whose
score is closest to `targetScore(difficulty)` = 2.5 + 2.2 × difficulty, and stops early within 10 %.

Scenario tests can load a generated level directly: `level gen:SEED:DIFFICULTY` ([testing.md](testing.md)).

## Test levels from ASCII

`tools/level/ascii2level.py IN.txt OUT` builds a level file from a drawing in the `levelcheck --map` legend.
Examples: `tests/levels/mechanisms.txt`, `tests/levels/rats.txt`, `tests/levels/bats.txt` (the built files sit next to them).

## Tests

- `tests/scenarios/mechanisms.txt`: key, gates, lever, rock falls (hit and dodged), keys kept over save and load.
- `tests/scenarios/rats.txt`: rat and giant rat screenshots (size, attack, die).
- `tests/scenarios/bats.txt`: bat and giant bat screenshots (roosting, swoops through the player, kill, fall).
- `tests/scenarios/generated.txt`: generated levels load (`gen:SEED:D` and campaign level 5), campaign end.
- `tests/scenarios/generated_path.txt`: plays `tests/levels/gen_d8` (seed 81, difficulty 8) from entrance to exit.
  Written by `levelcheck --script`.
