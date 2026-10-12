# Play a level or scenario from the command line

Status: draft 2026-10-12, ready (needs nothing first). From the user: start a hand-played game on a chosen level, or
on a scenario's setup, without playing through the campaign. Tooling for play-tests; the game rules do not change,
so once implemented and its checks pass it goes straight to `docs/plan/solved/` (AGENTS.md, Plans).

## Today

* `./game SCENARIO.txt` (`main`, `src/core/game.cpp`) runs a scenario: `Scenario::load` (`src/test/scenario.cpp`),
  virtual clock (`GameClock::enableVirtual`), dummy audio, the scenario's resolution, no `saves/settings.ini`. The
  script drives the input, then the game quits. Not playable by hand.
* No way to start a normal game on level N: New Game always starts at level 1.

## Idea

Two new flags. Both start a **normal, hand-played game**: real clock, sound, `saves/settings.ini` (resolution, keys,
options), the time-based random seed, mouse and keyboard from the player. They skip the main menu and go straight to
gameplay.

* `./game --level N` / `--level PATH` / `--level gen:SEED:D`: like New Game (fresh player), on campaign level N,
  any map file, or a generated level. Same parsing as the scenario `level` command (`loadLevel`,
  `loadGeneratedLevel`): reuse it, don't copy it. The level's exit leads on as in the campaign (level N+1; after a
  map file or a generated level, whatever New Game's flow gives, implementer's choice).
* `./game --play SCENARIO.txt`: play a scenario's setup by hand. The script's **setup commands** run, in order,
  in the first frame (no waiting): `seed`, `god`, `level`, `give`, `equip`, `wear`, `xp`, `learn`, `curse`, `poison`,
  `hurt`, `prop`, `riddles`, `chest`, `camera`, `motion`, `toon`, `hitboxes`, `killboss`, `hurtboss`,
  `poisonboss`, `poisonmonster`, `loadgame`. At the **first other command** (`wait`, `walk`, `hold`, `jump`, `key`,
  `screenshot`, `expect`, `quit`, ...) the script stops and the player takes over. `resolution` is ignored (the
  settings' resolution wins). `--play` needs a `level` (or `loadgame`) before the first other command; else it
  fails with a message, exit code 2.
  * So any existing scenario plays as "its level with its gear", e.g. `./game --play tests/scenarios/cobra.txt`.
* A bad flag or argument: print the usage and exit 2. The old form `./game SCENARIO.txt` stays as it is (the
  scenario runner and `make test` use it).
* Saving works as usual (normal `saves/`); these runs write nothing there unless the player saves.

## Docs

* `docs/testing.md`: a short "Play by hand" section with both flags and the setup-command rule.
* `docs/development.md` (where `make run` / running the game is described): one line per flag, link to testing.md.

## Tests

* Unit test (`tests/unit/`): the split of a scenario script into setup commands and the rest (which commands run,
  where it stops, the "needs a level" failure). Test the parsing of `--level` arguments if it is a function.
* `make`, `make format`, `make format-check`, `make tidy`, `make test` pass. The user play-tests the flags.

## Open (for the implementer)

* Where the code lives: a small play-mode path beside `Scenario` (`src/test/`) or in `main`; keep `main` short.
* Where the level exit leads after a map file or generated level.
