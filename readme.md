# Dungeon Crawl

A 2.5D side-scroller set in the dungeons of ancient Egypt. You play an archaeologist who climbs, jumps and
fights through tombs full of worms, scarabs, rats, bats, scorpions, cobras, crocodiles, man-eater plants, mimics,
mummies and Anubis guards. Find keys, pull
levers, avoid traps and collect the treasure.

My bachelor's degree work from 2011.

## What makes it different

**A dark tomb, lit by fire.** Torches, braziers and oil lamps are the only light. Flames flicker and smoke,
grit trickles from a loose ceiling just before the rock drops, and scarabs dig out of the sand. The world is
drawn in 3D on a flat grid, so you can look up, down and around. Press `F1` for a comic look with ink outlines.

**Choose the right weapon.** Club, sword, spear and bow deal blunt, slash or pierce damage. Each monster takes
some types badly and shrugs off others: a blade glances off a scarab's shell but a spear point goes between the
plates, and bronze armour dents under a club. Every weapon is the best answer to some monster, and an upgrade
helps the weakest weapon most.

**Not everything is what it seems.** A treasure chest can be a mimic. Bats hang asleep from the ceiling until
you come close. Mummies wait in their coffins for three thousand years and climb out as you pass. Most monsters
stop at the edge of a trap, so you can use the traps against them, but mummies and Anubis guards walk straight
through the spikes.

**Answer the sphinx.** Some gates are sphinxes that ask a riddle on a papyrus scroll. Type the answer to earn a
lot of experience. The riddles are classic sphinx riddles, palindromes, film and game trivia, and science
fiction. A hint shows after two wrong tries. You can add your own riddles as text files in `content/riddles/`.

**An archaeologist's notebook.** The journal (`J`) is a field notebook with pages you turn by hand. It fills in
only with what you have seen: a pencil sketch of a creature becomes a photo once you kill one, and its health and
weaknesses stay blank until you find them out. The riddles you met and the rules of the tomb are written down
too. The draft map (`M`) is a pencil sketch of the rooms you have explored.

**Locks, levers and portals.** Coloured keys and levers open gates of carnelian, turquoise, lapis and amber, and
later levels chain them together. Sphinx statues hold the portals between levels and teleporters to sealed
rooms. A boss guards every fifth level: Khepri, the mother of the scarabs, a vampire bat that heals by biting, the
scorpion queen, the serpent Apep, Sobek the crocodile god and Anubis himself, whose living statues step out of the
walls. The High Priest of Anubis waits on the level before him. The ankh behind Anubis wins the game.

**Grow stronger, learn Heka.** Kills, riddles and read inscriptions give experience; a new level brings more health
and stamina. Each slain boss gives a heka to spend on the Heka screen (`K`): the old magic of the gods, such as
Dodge, the Favour of Hapi in the water, Serket's favour against poison, the Blessing of Isis against curses, or
flaming arrows.

**Read the walls.** Hieroglyph inscriptions are copied into the journal. With the Words of Thoth skill you can read
them: lore, hints, now and then a lasting gift to your strength. A royal name carved in a cartouche opens a sealed
name gate to a secret room, if you can read it.

**Curses, poison and fire.** Mummies, priests and some beasts lay curses that weaken, slow or cloud your mind for a
while. Stings and bites poison; antidotes and amulets help. Fire burns through armour, but water puts it out.
Potions, amulets and weapon upgrades hide in chests, coffers, baskets, canopic jars and sealed pots, and not every
chest is just a chest.

**Die, and try again.** The game saves by itself at the start of each level, after each answered riddle and each slain
boss. When you die, Retry starts from that autosave, and Continue in the main menu does the same. Win the game to enter your name in
the Hall of Records.

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
| Next melee / ranged weapon       | `1` `2`                   |        |
| Drink healing / stamina potion   | `H` `0`                   |        |
| Drink antidote (when poisoned)   | `=`                       |        |
| Stamina skills on / off          | `Q`                       |        |
| Inventory                        | `I`                       |        |
| Draft map                        | `M`                       |        |
| Journal                          | `J`                       |        |
| Heka (skills)                    | `K`                       |        |
| Menu / back                      | `Esc`                     |        |
| Cartoon shading                  | `F1`                      |        |

These are the defaults: Options > Controls changes them (all but `Esc` and the F-keys). Options also has the
window size, fullscreen, the graphics effects and the volumes. The choices are kept in `saves/settings.ini`
([docs/settings.md](docs/settings.md)).

## Levels

The campaign has 30 levels, a boss every five. The gem in the top right corner shows the current level: carnelian for
levels 1-5, turquoise for 6-10, lapis lazuli for 11-15, malachite for 16-20, amethyst for 21-25, obsidian for 26-30. You can make your own with the level editor:
[tools/editor/readme.md](tools/editor/readme.md).

## Development

Build steps, tools, asset formats and tests: [docs/development.md](docs/development.md). Terms used in the game, the
code and the docs: [docs/glossary.md](docs/glossary.md).
