# Dungeon Crawl

A 2.5D side-scroller set in the dungeons of ancient Egypt. You play an archaeologist who climbs, jumps and
fights through tombs full of worms, scarabs, rats, bats, man-eater plants and Anubis guards. Find keys, pull
levers, avoid traps and collect the treasure.

My bachelor's degree work from 2011.

## Play

The game runs on Linux. Build it (see [docs/development.md](docs/development.md)), then start `./Play`.

| Action                           | Keyboard                  | Mouse  |
|----------------------------------|---------------------------|--------|
| Move left / right                | `A` `D`, `Left` `Right`   |        |
| Climb up / down (ladders)        | `W` `S`, `Up` `Down`      |        |
| Jump                             | `Space`                   | Right  |
| Sprint (hold)                    | `Shift`                   |        |
| Attack                           | `V`, `Enter`              | Left   |
| Interact: pick up, lever, riddle | `E`, `F12`                | Middle |
| Look around                      | `PgUp` `PgDn`, `Home` `End` | Move |
| Inventory                        | `I`                       |        |
| Menu / back                      | `Esc`                     |        |
| Cartoon shading                  | `F1`                      |        |
| Original models                  | `F2`                      |        |

The same list is in the game under Options.

## Levels

The campaign has 15 levels. You can make your own with the level editor:
[dungeon-editor/readme.md](dungeon-editor/readme.md).

## Development

Build steps, tools, asset formats and tests: [docs/development.md](docs/development.md).
