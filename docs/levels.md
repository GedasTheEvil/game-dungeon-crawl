# Levels: validator, generator, campaign

Level files, tile types and the editor: [tools/editor/readme.md](../tools/editor/readme.md).
Tile types and structures in code: `src/world/level.h`.

## Level format

A cell has three layers ([level-format-layers.md](plan/solved/level-format-layers.md),
[wall-layer.md](plan/solved/wall-layer.md)):

* **Structure**: `Wall`, `Empty`, `HalfWater`, `DeepWater`, `Sand`, `Quicksand`. A cell blocks when it is `Wall` or
  `DeepWater`.
* **Wall**: at most one fixture on the cell's back wall, with attribute and value: lever, inscription, wall spikes,
  sun disc (the wall types, `isWallType` in `src/world/level.h`).
* **Object**: at most one thing standing in the cell, with attribute and value: ladder, door, gate, key, treasure,
  trapped treasure, ankh, monster spawn, spike, death trap, rock fall, dart plate, light shaft, mirror, expedition page.

Two more say how the cell is dressed ([placed-decor](plan/solved/placed-decor.md)), each unset (the game scatters
it as on any level), bare (none) or one picked with its mirror, whatever the depth:

* **Decor**: the cell's prop (`DECOR_NAMES`, `src/world/decor.h`), among them the decoy, a coffin with a mummy asleep
  in it for good. A mummy's spawn keeps its own coffin, a page its skeleton, a living ushabti its decor ushabti.
* **Decal**: the cell's wall decal (`DECAL_DEFS`) and, as a second field, its wall torch (unset, none, a torch).
  Random torches keep their gap to a placed one.

Rules and the editor's palettes: [tools/editor/readme.md](../tools/editor/readme.md#decor-and-decal-layers).

Fixtures and objects stand in `Empty`, `HalfWater`, `Sand` or `Quicksand`. A cell can hold one of each: an inscription over a chest, a
lever behind a monster spawn. Interact takes the chest first, then the fixture (pull the lever, read the inscription)
with the next press, then a mirror (turn it); a gate (exit, teleporter) comes last. Wall spikes block their cell when out, so `levelcheck` warns
about a monster spawn, ladder, gate, door or treasure in front of them; a key or a floor trap there is fine. The map
screen shows one symbol a cell, the object's, else the fixture's.

Water rules (the game, `levelcheck` as far as it models them): the player wades through half water at half speed and
cannot sprint or jump while standing in it (the Favour of Hapi skill and the Blessing of Hapi potion allow both) (jumping into it is fine, so a pool can be a trap); a fall into it lands like on a
floor. Walkers wade too: slowed (scarabs, the worm, the mummy), unaffected (rats, Anubis), or faster (the crocodile, a
swimmer, 2.5 times its land speed; `Wading` and `KINDS` in `src/world/monster_kinds.*`). An arrow hits a monster standing in water for half its damage. Traps
work in it and show through the surface. `levelcheck` warns about half water that is not on deep water or a wall,
deep water not under water, a ladder that goes down into water (one may start in it), a crocodile, hippo, Sobek or Seth not
in or next to water and a lotus not in half water.

Sand rules (plan desert-chapter): a drift of sand (`,` in the structure drawing) slows the player's walk to 3/4 and
stops the sprint; jumping is fine. Quicksand (`%`) looks like sand (a slow swirl up close): standing in it the player
sinks, slower each second, stuck after 3 s, hurt from 5 s (5% of the max HP a second); a jump frees them, for twice a
jump's stamina. A sand fall (a rock fall with attribute 1, `&`) pours sand on whoever stands under it (40, a short slow)
and turns its cell to sand. A sand worm placed in sand lies under it and comes up under the player. `levelcheck`
counts sand and quicksand as a little more costly on the path, jumps a quicksand cell like a trap, and warns about
sand or quicksand not on a wall and a run of more than 2 quicksand cells.

Sunlight rules ([sunlight-mirrors](plan/solved/sunlight-mirrors.md), `src/world/sun_beam.h`): a light shaft (`l`, attribute: its way, 0 down, 1
left, 2 up, 3 right) sends a beam of sunlight through open cells until rock, deep water or a gate not fully open stops
it; the player does not. A mirror (`(`, value: the way it points) sends a beam that falls on it on that way: pointing
the way the beam goes it stands edge-on and lets it by, pointing back it catches it. Interact turns a mirror a quarter
turn clockwise (down, left, up, right). A sun disc (`a`, a fixture, attribute: lock colour) a beam falls on is lit for
good and opens every gate of its colour, as a lever. A beam burns mummies standing in it (fire, `SUN_BURN_FIRE`) and
counts as light for the camel spiders. `levelcheck` models the beam: turning a mirror lights every sun disc a beam can
reach with the mirrors the player reaches from there (with the colours open by then) set as needed; it warns about a
light shaft without rock behind it, a mirror in front of an inscription, a sun disc no beam can ever reach and one lit
as the level starts.

Level files are text, version 5: a `DCLEVEL 5 40 47` header, the level's zone (`zone cisterns_of_sobek`, or
`zone none`; [Zones](#zones)), an optional level title (`title The Hall of Drifts`, the rest of the line; every campaign
level has one, plan campaign-texts), the structure as a drawing, then the lists of fixtures, objects, props and decals
(format: [tools/editor/readme.md](../tools/editor/readme.md#file-format)). Save games hold
the same block for the current level, with the fixtures' live state (a pulled lever, the wall spikes' state). Old (v1
to v4) level files and saves still load, converted as they are read (a v2 file's wall types move to the wall layer, a
v3 file's cells are all unset, a v4 file has no zone); `levelcheck` warns about a file older than v5, and
`bin/levelconvert FILE...` (built by `make`) rewrites one as v5. Water: [crocodiles-and-flooded-cells.md](plan/solved/crocodiles-and-flooded-cells.md).

## Campaign order

`src/world/campaign.h`. A game plays `content/levels/lvl1` to `content/levels/lvl35` (`CAMPAIGN_LEVELS`), a boss every five
levels ([longer-campaign.md](plan/solved/longer-campaign.md)). Each exit loads the next level. `content/levels/lvl35` holds the ankh that wins the game; the Anubis boss there is always the last boss.
Levels 15-35 use the whole 40 x 47 grid ([denser-levels.draft.opus.md](plan/denser-levels.draft.opus.md)).

### Zones

The levels are grouped into zones ([campaign-zones](plan/solved/campaign-zones.md), `ZONES` in
`src/world/campaign.h`, `zoneOf(level)`; what each zone is, by `ZoneId`: `ZONE_INFO`). Each level file names its zone
too (`zone SLUG`, `LevelMeta`; the editor's zone button): the game dresses and lights the level by the file's zone, a
unit test checks it is the one of the level's range. A level without one (`zone none`, a test or generated level)
keeps the tombs' look. A zone is a run of levels with its own stone, main monsters, new mechanic and, mostly,
a boss. Each zone is one hour of Ra's night journey through the Duat (the Book of Gates); its title names the hour and
its gate: "The Fourth Hour: the Gate of the Priests" (`zoneTitle`, the hour is the zone's place in the target table).
A zone's boss is the kin of its monsters (AGENTS.md, Boss abilities).

Each campaign level also has a title of its own in its file (`title`, `LevelMeta::title`; `levelcheck` warns about a
campaign level without one): "The Hall of Drifts" is a level of "The Eighth Hour: the Gate of Seth". The level banner,
the map and the save slots show it. On entering a zone the journal gets the hour's field note
(`FieldNote::HourFirst` to `HourTwelfth`, by `ZoneId`), in the archaeologist's voice.

Each zone looks and sounds its own ([zone-looks](plan/solved/zone-looks.md)): its stone dresses the walls,
floors and ceilings (`scatterSurfaces`; plaster is limestone painted more often, wet stone, basalt and granite have
their own textures and few rough rows), and `ZONE_LOOKS` (`src/world/zone_look.h`, by `ZoneId`, all twelve) sets its
decor set (a factor per prop and decal on the tier weights), the ambient light, a haze far from the camera, the
chance of a wall torch, an ambient loop (`media/sounds/ambience/`, `tools/audio/zone_ambience.py`) and the guardian
its banner names. On a zone's first level a banner shows its hour and gate, the zone and its guardian; on every other
level a smaller one, the level and its zone (`ZoneBanner`, `levelBanner`). A level outside the campaign keeps the
tombs' look (`DEFAULT_LOOK`).

Today's 35 levels, as the code table maps them:

| Zone | Levels | Stone | Main monsters | New mechanic | Boss |
|---|---|---|---|---|---|
| 1 Tomb entrance (the Gate of the Tomb) | 1-5 | limestone | rats, scarabs, bats, plants, worms, scorpions | spikes, pits, the first key and riddle | Khepri (lvl5) |
| 2 Bat galleries (the Gate of the Bats) | 6-10 | limestone | bats, giant bats, giant rats, giant scarabs, crocodiles, camel spiders | levers, rock falls, the first water | Bat of Shezmu (lvl10) |
| 5 Scorpion nest (the Gate of Serqet) | 11-15 | plaster | mummies, ushabti, giant scarabs, giant bats, scorpions, giant scorpions | poison | scorpion queen (lvl15) |
| 6 Serpent temple (the Gate of the Serpent) | 16-20 | plaster | cobras, mummies, Anubis guards, crocodiles, giant bats | dart plates | Apep (lvl20) |
| 8 Seth's desert (the Gate of Seth) | 21-25 | sandstone | sand worms, camel spiders, giant scorpions, giant cobras, giant bats, hippos | sand, quicksand, mirrors and sun discs | Seth (lvl25) |
| 7 Cisterns of Sobek (the Gate of Sobek) | 26-30 | wet stone | crocodiles, giant cobras, hippos, mummies, Anubis guards, giant bats | deep water, swimming | Sobek (lvl30) |
| 11 Necropolis (the Gate of the Dead) | 31-34 | granite | Anubis guards, mummies, giant scarabs, giant cobras, giant scorpions, cursed mimics | every trap mixed | High Priest (lvl34, until the rebuild) |
| 12 Hall of Judgment (the Gate of Judgment) | 35 | granite | Anubis guards, mummies, giant cobras, cursed mimics | riddles | Anubis (lvl35) |

The target campaign (decided 2026-10-21): twelve zones, about 54 levels, four acts. The rebuild
([denser-levels](plan/denser-levels.draft.opus.md), [campaign-levelgen](plan/campaign-levelgen.md)) builds
it zone by zone and then switches the code table to it. Level ranges are about, not exact.

| # | Zone | Levels | Monsters | New mechanic | Boss |
|---|---|---|---|---|---|
| **Act I: the tomb** (limestone) |||||
| 1 | Tomb entrance | 1-4 | rats, scarabs, bats, plants | spikes, pits, the first key and riddle | Khepri |
| 2 | Bat galleries | 5-9 | bats, giant bats, worms, giant rats | levers, rock falls, a short first wade (one crocodile) | Bat of Shezmu |
| 3 | Web halls | 10-13 | camel spiders, giant scarabs, mimics | darkness and light (spiders shun torches), trapped chests | spider boss ([camel-spider-boss](plan/solved/camel-spider-boss.md)) |
| **Act II: the temple** (painted plaster) |||||
| 4 | Priests' quarter | 14-18 | mummies, living ushabti, the first Anubis guards | name gates, many inscriptions, fire jets (braziers), ring transporter | High Priest |
| 5 | Scorpion nest | 19-22 | scorpions, giant scorpions | poison drips, antidotes | scorpion queen |
| 6 | Serpent temple | 23-27 | cobras, giant cobras | dart plates, rhythm traps | Apep |
| **Act III: the river and the sands** |||||
| 7 | Cisterns of Sobek (wet stone) | 28-32 | crocodiles, hippos, giant cobras | deep water, swimming, Hapi potions | Sobek |
| 8 | Seth's desert (sandstone) | 33-37 | camel spiders, giant scorpions, hippos at the oasis | sand, mirrors and sun discs | Seth |
| 9 | Land of Sokar (sandstone) | 38-41 | sand worms, camel spiders in the dark | quicksand, sand falls, deep drifts | worm boss ([worm-boss](plan/solved/worm-boss.md)) |
| **Act IV: the Duat** (dark granite) |||||
| 10 | Lake of Fire (black basalt, red light) | 42-45 | baboons ([baboon](plan/solved/baboon.md)), fire-spitting cobras, giant bats | fire everywhere: fire-jet runs, burning floors, braziers; water as the refuge | Babi ([babi-boss](plan/solved/babi-boss.md)) |
| 11 | Necropolis | 46-50 | Anubis guards, mummies in coffins, cursed mimics, a few baboons | every trap mixed, the Stargate chamber, the last fork | none |
| 12 | Hall of Judgment | 51-54 | Anubis guards, the best of every zone | riddle-heavy finale, the weighing of the heart | Anubis |

Why this order and its costs: [campaign-zones](plan/solved/campaign-zones.md).

### Levels

| Levels | Content |
|---|---|
| 1 | The way in: rats and a scarab, spikes, a spike pit to jump, ladders, the first chests. |
| 2 | The scarab galleries: scarabs, bats, plants by the chests, a riddle room up a ladder, a drop shaft, two spike pits. |
| 3 | The worm tunnels: worms, rats, a bat, the first scorpion, a rock fall, the first key (red) and gate. A riddle gate in the exit hall. An antidote in a niche past the shaft at the east end of the upper hall, a lesser resistance potion in the lower hall. |
| 4 | The treasury: rats, scarabs, a scorpion, a worm, plants guarding side rooms, the first mimic among real chests. A riddle gate. An antidote at the east end of the lower hall, a lesser resistance potion in the middle hall. |
| 5 | The scarab king, the first boss: a teleporter to the sealed boss room, the boss scarab and its scarabs. Behind the boss gate the blue key for the blue gate before the exit. A riddle room past a spike pit. A scorpion in the exit hall, an antidote beside the ladder down to it, a lesser resistance potion in the lower hall. |
| 6 | Rats, a giant rat, the first giant scarab, two scorpions. Red key and gate. An antidote where the shaft lands in the lower den, a lesser resistance potion in the middle hall. |
| 7 | Bats and giant bats in low tunnels. Blue lever and gate. The first water: a flooded stretch of the west hall, the first crocodile in it, on the way to a chest. |
| 8 | Plants, worms, giant rats, giant scarabs. Red key, then the green key behind the red gate. The hall to the red gate is flooded, a crocodile in it. |
| 9 | Giant bats. Red key, red gate, then the blue lever behind it for the blue gate. Past the pit in the lower gallery a flooded stretch with a crocodile. |
| 10 | The vampire's roost, the second boss: the teleporter at the east end of the upper hall leads to the sealed roost, the vampire bat and its bats. Behind the boss gate the gold key for the gold gate to the shaft down. A giant scarab by the exit. |
| 11 | Giant rats, giant bats, giant scarabs, the first two mummies. Two levers (red, blue) in two halls open two gates in a row. |
| 12 | Plants, giant bats, three mummies. Chain: red key, green lever, gold key. |
| 13 | Rock falls, giant scarabs, giant rats, a mummy, the first Anubis. Blue key, gold lever. |
| 14 | Giant scarabs, rats and bats, three mummies, two Anubis. Red key, blue lever, green key. |
| 15 | The scorpion queen's nest, the third boss: giant scorpions and scorpions in every hall, mummies, giant scarabs, giant bats, an Anubis. The blue lever opens the way down to the nest, a cavern with two ledges (green key, green gate). The teleporter leads to the sealed queen's chamber: the scorpion queen between two egg clusters her brood hatches from; behind the boss gate the gold key for the gold gate to the spike pits and the exit cavern. Antidotes in the second hall and past the pits, a lesser resistance potion in the third hall, a greater one by the gold gate. |
| 16 | The serpent temple's gate: cobras, crocodiles, mummies, two Anubis, giant bats. A loop of two halls round a pillar (red key, red gate, a riddle chamber), a flooded hall with a deep crocodile pool (blue lever), a rock-fall gauntlet with a drop / ladder loop to the green key; the gold lever in a side gallery opens the exit hall's gold gate. The first dart trap, in the upper loop hall, a mummy east of it. |
| 17 | The cistern: the entrance hall forks into two wings, the west one to the red key and the blue lever (an Anubis), the east one behind the red gate down into the flooded cistern (crocodiles, a swimming cobra, the green key on a ledge). The rock-fall drain, the pool hall, the gold lever, death pits before the exit. A riddle shrine and a snake pit between the wings. |
| 18 | The sanctuary of the uraeus, climbed bottom to top: the hall of two gates (red lever below, blue lever above), a pillar loop with the green key inside the pillar, a rock-fall gauntlet, a 3-high flooded hall with crocodiles and a riddle gate, the gold key by an Anubis. |
| 19 | The serpent sanctum: cobras, giant bats, mummies, five Anubis, two crocodiles in a two-high flooded hall. Red key, blue lever, green key past a drop shaft, gold lever beyond a loop round a pillar, then a spike-pit gauntlet before the exit hall. |
| 20 | Apep's pit, the fourth boss: cobras, mummies, Anubis, Apep's throat (a shaft from the second hall down to the fifth), a crocodile pool. Red key, blue lever, green lever; the teleporter in the ninth hall's east nook leads to the sealed pit: a sunken floor between two ledges, Apep and his cobras, the boss gate on the east ledge, behind it the gold key for the gold gate before the exit. |
| 21 | The buried gate, the first desert level ([desert-chapter](plan/solved/desert-chapter.md)): sandstone halls, drifts of sand in every hall (slower, no sprint), the first quicksand (single cells), sand falls, sand worms under the drifts, camel spiders, giant scorpions, giant cobras, giant bats, mummies. The first sunlight mirror ([sunlight-mirrors](plan/solved/sunlight-mirrors.md)): in the fourth hall a shaft of sunlight falls through a crack onto a mirror facing it; turned west, the beam lights the red sun disc at the hall's west end (and burns the hall's mummy). Red gate; blue key, blue gate. A riddle gate at the east end of the eighth hall. |
| 22 | The hall of drifts, started from the east: long sand halls, two of them two high, quicksand in pairs, sand worms, camel spiders, giant scorpions, giant cobras. The desert's first dart trap. Red key, red gate; gold lever, gold gate. A riddle gate, a name gate to a sealed room under the eleventh hall. |
| 23 | The sand stair: twelve halls down in a zigzag, each shorter than the level, sand falls over the drifts, quicksand pits, a dart trap, giant cobras, giant scorpions, camel spiders. Blue lever, blue gate; green key, green gate; a riddle gate. A drop into the exit cavern. |
| 24 | The house of Seth's servants: a buried temple, Anubis guards and mummies among the drifts, sand worms, giant scorpions. Red lever, red gate; gold key, gold gate; a riddle gate. A name gate to a sealed room over the exit cavern. |
| 25 | Seth's oasis, the fifth boss: eleven halls of drifts and quicksand to the teleporter (red key, red gate; blue lever, blue gate; a riddle gate). It leads to the sealed oasis: Seth ([hippo-boss](plan/solved/hippo-boss.md)), the red hippo, in his pool between two sand ledges, his hippos come up out of the water; the Blessing of Hapi by the teleporter. Behind the boss gate the gold key and the ladder up to the exit hall (the gold gate before the exit). |
| 26 | The flooded halls: giant cobras, six crocodiles in long wades, giant bats, mummies, Anubis. Red lever into the two-high flooded hall, blue key, green key past the riddle, gold lever by the last crocodile; a teleporter to a hidden treasure cistern. |
| 27 | The cistern grid: halls split by a central wall and joined by full-width passages; giant cobras, crocodile pools, mummies, Anubis, giant bats. Red key, blue lever, green lever down a dead end, gold key behind a mummy; a rock-fall gauntlet, spike pits before the exit hall. |
| 28 | The cistern stairs: fifteen halls down in a zigzag, five of them flooded; crocodiles, giant cobras, giant bats, mummies, Anubis. Red key, blue lever, green key, gold lever. A dart trap in the blue lever's hall, an Anubis coming over it. |
| 29 | The twin cisterns: down the west wing, across a flooded hall, through a sunken crypt, up the east wing to the exit. Gold lever, red key, blue lever in the crypt; the green key near the top opens the exit gate. A bridge over a well joins the wings. |
| 30 | Sobek's lake, the sixth boss: crocodiles in every pool, giant cobras, Anubis guards, a loop round a pillar with a red gate on each side. The blue lever opens the gate to the teleporter: the sealed flooded lake with two rock shelves, Sobek and his crocodiles; behind the boss gate the gold key for the gold gate on the way to the exit. |
| 31 | The necropolis: Anubis guards, mummies in a coffin row, giant cobras, giant scarabs, giant scorpions, loose ceilings everywhere. A processional stair as a hub with wings: red key, red gate, blue lever, blue gate, green key, green gate; the gold lever before the exit. |
| 32 | The necropolis gate: a loop of two halls round a pillar with a hidden chamber inside, a flooded crypt with giant cobras, a rock-fall gauntlet and a riddle room. Red key, blue lever, green key, gold lever (two gold gates: the way down and a treasure tomb). |
| 33 | The catacombs: the hall of coffins (3 high, two ledges, the red lever), the blue key, a flooded tomb, the green lever (two green gates), the gold key on a ledge in the 3-high hall of Anubis. |
| 34 | The two towers of the dead: the red key opens the bridge to the east tower, the blue lever there the way back west into the great flooded hall (giant cobras in the water, the green key on an island); the gold key, the Anubis hall, a rock-fall gauntlet down to the teleporter, a drop to a hidden tomb. The teleporter leads to the sealed tomb of the High Priest of Anubis (the mummy boss, plan mummy-boss; a temporary home until the campaign is re-planned): his mummies climb out of the coffins round him, the exit behind the boss gate. |
| 35 | The ankh chamber, the finale: four lock halls (red key, blue lever, green key past a flooded stretch, gold lever), a hall of coffins and two rock-fall gauntlets down to the teleporter. It leads to the sealed 3-high ankh chamber: the Anubis boss, his guards stepping out of the walls; behind the boss gate the treasure and the ankh that wins the game. A long ladder past the chamber wall to a bottom treasure gallery. |

Trapped chests ([trapped-chests](plan/solved/trapped-chests.md), `%`): one a level in 3-12, two in 13-24, three in 25-35,
each a chest that held a plain potion (health, stamina, might, armour). Never put what the level needs in one (a key
item, a new weapon, the antidote, a Hapi potion): half of them are empty, and the checker does not count a trapped
chest's antidote as in reach.

The earlier expedition's notebook ([expedition-notes](plan/solved/expedition-notes.md)): 24 torn pages (`=`,
attribute: the page number, in order of depth; `EXPEDITION_PAGE_COUNT`, texts in `content/texts/expedition.txt`, plan
campaign-texts). 18 of them (1-6, 8, 10, 11, 13-15, 17, 18, 20, 21, 23, 24) lie on skeletons in levels 2, 3, 4, 6, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23,
24, 27, 31 and 34, each a level or so before what it hints at, off the exit path; the last one (24) lies on lvl34's
path. The pages 7, 9, 12, 16, 19 and 22 (webs, an early fork, poison drips, deep water, worms and a fork, the lake of
fire and a fork) are written but not placed: the rebuild places them, one every two or three levels. Some pages
hint at a fork ahead; the last stays the last fork's warning.

Scorpions live in levels 3-6, 15 and 21, giant scorpions in 15, 21-25 and 31-35, cobras in 16-20, giant cobras in 21-35; each level with a poisoner has an antidote in reach (the checker warns otherwise). Lesser resistance potions lie on the poisoner levels from 3 on, greater ones from 15 on (one of each per level from 15). One Blessing of Hapi potion (water blessing, plan water-blessing-potion) lies in a chest on the approach to the first half water of each water level (7-9, 16-20, 25 (by the oasis), 26-30, 32-35), a few cells before it; levelcheck assumes none. Crocodiles live in the water of levels 7-9 (and many flooded levels after). Hippos lie in the water of levels 16, 18, 26 and 28 (one a level, in a long pool; levelgen makes no water); Seth, their boss, in the oasis of 25. Blue lotuses float in the water of levels 7-9, 16-20, 25, 26-30 and 32-35 (one to three a level; levelgen makes no water). Camel spiders run in levels 4 and 6-8 (one a level) and in the dark desert halls of 21-25 (three to five a level; levelgen from difficulty 3 to 8); they keep out of the light of torches, braziers and lamps. Living ushabti stand among the decor ushabti in levels 11-14 (one a level, two in 12; levelgen from difficulty 6 to 10), and step out of the wall with the Anubis boss's guards in 35. The desert (21-25, plan desert-chapter) has sand: drifts on the floor (the player walks at 3/4 speed and cannot sprint), quicksand (single cells or pairs), sand falls, and sand worms lying under the drifts that come up under the player. Cursed mimics (plan cursed-mimic) lie as dark chests in levels 27, 29, 31, 33 and 35 (one a level, beside a treasure, off the path; plain mimics in 4, 15, 17-22, 24, 26, 27, 32 and 33; levelgen makes 30% of its mimics cursed from difficulty 8). Baboons (plan baboon) keep their distance and throw burning coals in the necropolis halls of 31 (three), 32 and 33 (two each), on long open rows; their home, the Lake of Fire, comes with the rebuild (levelgen from difficulty 9). Their boss Babi (plan babi-boss) has no campaign slot yet: his arena is `tests/levels/babi`. Mummies appear from level 11 on, Anubis from level 13. Every level from 6 on has one or two
[inscriptions](inscriptions.md) (`content/inscriptions/campaign.txt`): real texts, some with a hint for the level (the boss
ahead, a dart trap, a hidden treasure room, the order of the locks). Levels 6, 8, 10, 12, 14, 16, 18, 22, 24, 26, 27, 29, 31,
33 and 35 have a secret room ([names](names.md), plan secret-name-gates): a name gate (`!`) off the exit path, the name
cartouche (`:`) elsewhere on the level, and the sealed room carved into the rock, reached only by the name gate: its
teleporter, a lore inscription (ids 33 to 45, 51 and 52, two with a boon), a chest with a rare potion or an amulet (two in some),
a mummy guarding it in 14, 26, 31 and 35. Weak monsters give way to their giant kin: no rats, scarabs or small bats
after level 9, no small scarabs after level 5 (giant scarabs from 6), except a boss's minions (the vampire bat's bats in 10).

The level files in `content/levels/` are the only source of truth. The ASCII drawings in `tools/level/campaign/` were
the first helpers for each level and are not kept in step: never rebuild a campaign level from them. Change a level in
its file (by hand or in the editor). A drawing is a start for a new level only ([Test levels from
ASCII](#test-levels-from-ascii)); once built, the level file is the one to edit.

## levelcheck: validate and rank

```
make level-tools
bin/levelcheck content/levels/lvl*                   # report per level, then a ranking, easiest first
bin/levelcheck --map content/levels/lvl3             # plus the map with the path drawn as '*'
bin/levelcheck --legend                      # the map's characters, with the tile each one stands for
bin/levelconvert content/levels/lvl*                 # rewrite v1 to v3 level files as v4 (v4 files are left alone)
bin/levelcheck --script tests/out/paths content/levels/lvl*   # a scenario per level that plays the path
```

Exit code: 0 if all levels are valid, 1 if one cannot be finished, 2 for a usage error or an unreadable file.

The check walks the level with the player's movement rules from `Dungeon` (`src/world/level_check.h`):

- Walk left or right into any open cell. With no floor below, fall straight down to a floor or a ladder.
- Climb between vertically adjacent `Ladder` cells.
- Jump over one cell: a gap in the floor, or spikes or a death trap on it, from a floor (a ladder's foot too), not out
  of half water. The
  jump peaks 0.4 tiles high (`Jump::ARC`, `src/world/movement.h`, from the game's jump constants), so the player
  cannot step up onto a ledge. A rock fall is walked, not jumped (it drops all the same). Jumps from mid-ladder are
  not modelled. Stamina is not counted: it comes back while the player waits.
- A key, or pulling a lever, opens every gate of its colour. Turning a mirror lights the sun discs a beam can then
  reach (the sunlight rules above): their gates open. In the level file a gate is closed only with value 0
  (the game opens one saved while opening), and a lever with value 1 is already pulled and opens nothing.
- Reaching the boss counts as killing it (the checker cannot fight): the boss gates (lock colour 5) open. No key
  opens them.
- A teleporter (Door, gate type 5) jumps to the other teleporter with the same pair id, both ways. A name gate (a
  teleporter whose value has `NAME_SEAL`, 256, added to its pair id) too, but the exit, the keys, the levers and its
  name cartouche must also be found with it sealed.

It reports:

| Item | Meaning |
|---|---|
| errors | No entrance, an entrance with no floor below it, no exit or ankh, or the exit cannot be reached. The level is invalid. |
| warnings | More than one entrance, keys, levers, treasure or expedition pages out of reach, an expedition page without a number, gates that nothing opens (no key, lever or sun disc), the sunlight rules above, a teleporter pair id without exactly two teleporters, a boss gate without a boss or a boss without one, more than one boss, a boss the player can walk to (not only by teleporter), softlock cells, a path that must cross a death trap or spikes, an object or fixture in a wall or deep water, a sphinx gate that is no entrance, exit, riddle or teleporter, the water rules above, a monster spawn, ladder, gate, door or treasure in front of wall spikes, wall spikes without a floor under them or a wall above them, a fire vent without a floor under it or in water, a poison drip without a floor under it or a ceiling over it, in water or with an object in its puddle (but a key or a page), a run of rhythm traps (wall spikes, fire vents, poison drips) that cannot be walked through both ways, sand or quicksand not on a wall, a run of more than 2 quicksand cells, a level file older than v4, a placed prop, decal or torch that does not fit ([editor readme](../tools/editor/readme.md#decor-and-decal-layers)), a poisoner (scorpion, cobra), a poison drip or a dart trap on the path without an antidote in reach, an inscription out of reach or without a text id, more than one name gate or name cartouche, a name gate without a name cartouche or one
without a gate, the exit, a key or a lever only behind a name gate, a name cartouche found only through its gate or next
to it (2 cells along the row). With the files: an inscription whose text id has no text in `content/inscriptions/`, the same text id in two places. |
| size | Open cells, reachable cells, the bounding box. |
| content | Monsters by type, traps, treasure, keys, gates, levers, riddles. |
| path | The cheapest route: moves, jumps, drops, ladder steps, and the hazards, gates and monsters on it. |
| difficulty | The score used for ranking. About 5 is easy, 12 is medium, 20 or more is hard. |

A softlock cell is a cell the player can reach but cannot leave for the exit, for example the bottom of a
one-way drop. The bottom of a spike pit counts as a death, not as a softlock.

Difficulty score (`difficultyScore` in `level_check.cpp`): 0.04 per path move, 1 per spike, 4 per death trap,
3 per rock fall, 2 per dart plate, 1.5 per wall spikes, fire vent or poison drip cell and 1.2 per jump on the path, 0.8 per gate and per mirror
turn, and 1 more for a jump over a death pit.
Monsters add their threat (`monsterThreat`: rat 0.7, scarab 0.8, bat 1.2, plant 1.5, worm 2, giant rat 3,
giant bat 3.5, giant scarab 4, crocodile 4.5, mimic 2, mummy 5, Anubis 10, boss scarab 10, vampire bat 12, anubis boss 16, scorpion 1, cobra 3, giant cobra 5, giant scorpion 3.5, egg cluster 0.5, scorpion queen 13, Apep 14, Sobek 14.5, lotus 2, mummy priest 15, camel spider 2.5, hippo 6, ushabti 1.5, cursed mimic 4, Seth 16, baboon 5.5, Rerek 16.5, Mother of Webs 12, Babi 16; a kiter's threat x1.15):
the full value within 3 cells of the path, a quarter elsewhere; a climber (it follows the player up and down ladders:
rats, scorpions, mummies, Anubis and their giants and bosses) 5% more. Each reachable treasure takes 0.2 off.

The ranking keeps the finale (a level with the ankh) last.

### Playing the path in the game

`--script DIR` writes `DIR/<level>.txt`, a [scenario](testing.md) that plays the path in god mode and expects the
level to be finished (`expect level == 2`, or `expect won == 1` for the finale). Run it with
`make test SCENARIO=DIR/<level>.txt`; `make paths` does it for every campaign level. It checks the movement model
against the real physics.
Monsters and damage do not stop it, so it proves that the level can be crossed, not that it is fair.

## levelgen: random levels

```
bin/levelgen --seed 7 --difficulty 5 --map content/levels/test1    # one level, print the map
bin/levelgen --seed 1 --difficulty 3 --count 5 /tmp/gen    # /tmp/gen1 .. /tmp/gen5, seeds 1 .. 5
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
   worm 3-7, plant from 3, camel spider 3-8, giant rat from 4, giant scarab from 5, giant bat and living ushabti from 6,
   mummy from 7, Anubis from 9.
   At most 4 + 2 × difficulty monsters. Weapon chests come in by depth like in the campaign: club and dagger, the
   short sword and spear from difficulty 3, the khopesh from 5, the axes and the mace from 7; self-bow and sling, the
   throwing stick and javelin from 4, the composite bow from 7.
   From difficulty 2, 15% of the treasure chests are mimics, drawn from a separate random stream; from difficulty 8,
   30% of the mimics are cursed. Most seeds
   keep their layout; where a mimic changes the difficulty score, the generator may pick another candidate.
   A mimic is `M` in the legend; the draft map shows it as a chest. A giant scarab is `k`, the boss scarab `K`, the vampire bat `V`, the Anubis boss `N`, a mummy `u`, a boss gate `Z`, a teleporter `O`, a name gate `!`, a name cartouche `:`, a light shaft `l`, a mirror `(`, a sun disc `a`.

Each candidate goes through `checkLevel`. The generator only keeps a level that is valid and has no warnings
(so no softlocks, and every key, lever and treasure is in reach). Of up to 80 candidates it returns the one whose
score is closest to `targetScore(difficulty)` = 2.5 + 2.2 × difficulty, and stops early within 10 %.

Scenario tests can load a generated level directly: `level gen:SEED:DIFFICULTY` ([testing.md](testing.md)).

### Zone levels

```
bin/levelgen --seed 3 --difficulty 6 --zone necropolis --map /tmp/zone    # a zone level, about 650 open cells
bin/levelgen --seed 3 --difficulty 6 --zone web_halls --cells 500 /tmp/zone    # a smaller one
bin/levelgen --seed 3 --difficulty 6 --zone cisterns_of_sobek --boss /tmp/zone    # the zone's boss level
```

With `--zone SLUG` (a zone's slug, [Zones](#zones)) levelgen builds a level in the campaign's shape
([campaign-levelgen](plan/campaign-levelgen.md), all 9 steps: the shape, the terrain, the monsters,
the traps, the gates, the boss levels, the supplies, the carried content and the decor) and names the zone in the file,
so the game dresses it as the zone. Scenarios: `level gen:SEED:DIFFICULTY:ZONE`. With `--boss` (scenarios:
`gen:SEED:DIFFICULTY:ZONE:boss`) it builds the zone's boss level (step 6, see Boss levels below).

1. **Halls.** Bands from the top of the grid to the bottom: halls of 1 (55%), 2 (30%) or 3 rows (15%), most of
   the width across (their lengths scaled to the target size, `--cells`, `ZONE_OPEN_CELLS` = 650 by default), two
   rows of rock between them, one now and then, three to five over a long shaft. A quarter of the levels is climbed
   from the bottom.
2. **Route.** The entrance at one end of the first hall; each hall crossed to a link at least half its length away,
   a ladder down (a quarter of them a drop), or up on a climbed level; the exit at the far end of the last hall.
   Up to 2 hub halls the shaft runs straight through (from a ladder, a wing either side).
3. **Pillar loops.** Where the route crosses a hall one way and the next one back, a second ladder between them
   (up to 3 a level): two ways round the pillar. No gate goes between the two ladders.
4. **Wings and ledges.** A hall's ends off the route are wings. In a tall hall a long wing can end in a ledge (up to 3
   a level): its last 3 to 5 cells raised a row, up a ladder.
5. **Locks.** One gate more than a test level of the difficulty (1 to 4), its key or lever at the end of a wing
   before it: on its hall before the gate, or on a hall before it with no drop on the way (else a branch, as on a
   test level). A chain where it can: the wing past the nearest gate already placed before it.
   Then the zone's other gates (`ZONE_GATES`, `src/world/campaign.h`; step 5): riddle gates (one a level, three in
   the Hall of Judgment), half of them in a riddle room up a ladder, else at a wing's end; now and then (by the
   zone's chance, none in the tomb entrance) a name gate at a wing's end to a secret room sealed in the rock (5 to 7
   cells, a rare chest: an amulet of the depth's tier or a rare potion), its name cartouche on a hall of the route;
   hidden rooms behind a plain teleporter (one or two chests, or a guard from difficulty 6); and treasure tombs, a
   wing past a gate closed by a second gate of its colour, a rare chest at its end.
6. **Terrain.** By the zone table (`ZONE_TERRAIN`, `src/world/campaign.h`; step 2): water (few: one short wade;
   some: two or three wades and a pool; much: five to eight runs, three pools and two or three flooded halls), sand
   (few: drifts; some: drifts with quicksand, one or two cells; much: also sand falls, half the rock falls), dark halls
   (few: one or two; some: 40%; much: 75%; no wall torch but one every 8 to 16 cells), braziers (two to five),
   daylight cracks (light shafts down onto rock, two to four) and sun discs. Half water lies on rock or, in a pool, on
   one or two rows of deep water dug into the rock under the floor; never within 2 cells of a drop (the jump over
   its hole starts on dry floor) nor on a ladder's top. In a zone with mirrors, 60% of the gates are opened by a sun
   disc at the end of a wing in a tall hall: a light shaft in the hall's top row falls on a mirror pointing back up
   at it (it catches the beam), the player turns it to the disc.
7. **Monsters.** From the zone's set (`ZONE_FAUNA`, `src/world/campaign.h`; step 3), the target campaign's
   ([Zones](#zones)), each by its weight: crocodiles, hippos and lotuses only in half water and likelier there, sand
   worms three times likelier on a drift (they lie under it), camel spiders three times likelier in a dark hall and
   a quarter as likely in a lit one (in a zone with dark halls). Half the mummies lie between decoy coffins (a coffin
   with a sleeping mummy, `DECOR_COFFIN_MUMMY`, either side), every living ushabti among decor ushabti. The share of
   the chests that are mimics, and of the mimics that are cursed, is the zone's too.
8. **Traps.** By the zone table (`ZONE_TRAPS`, `src/world/campaign.h`; step 4): the weight of spikes, death pits
   and rock falls in percent of a test level's, and of dart plates, wall spikes, fire vents and poison drips (none on
   a test level) along the halls; rock falls, wall spikes and drips only under a one-row ceiling, spikes and plates
   on dry floor. A rhythm trap gets its own phase, a fifth of them slow, from difficulty 6 almost a third fast. Then
   the zone's trap gauntlets (0 to 2 a level): a stretch of 7 to 13 cells of a hall the route crosses (off a pillar
   loop, preferably under a one-row ceiling) with a trap of the zone's kinds on every other cell, a free cell
   between to wait in; from difficulty 6 now and then two rhythm traps side by side, kept only where the checker
   finds them crossable both ways. `levelgen` reports the counts. The path scripts (`levelcheck --script`) wait
   before a run with wall spikes (`waittraps`, [testing.md](testing.md)).
9. **Boss levels** (`--boss`; step 6). The zone's boss (`ZONE_BOSS`, `src/world/campaign.h`; none in the
   necropolis) in a room of 18 cells sealed under the halls (on row 2, the halls start over its ceiling). The last
   gate stands on the last hall's crossing, in the colour a boss level keeps free (one lock fewer); a teleporter at
   the end of one of the last free wings before it leads to the room. From the room's teleporter: the boss in the
   middle, then the boss gate and behind it the last gate's key and a rare chest, in an alcove one row tall. The
   room by the boss: one row for Khepri, the High Priest (the floor round him free for his coffins) and Rerek (a run
   of sand); three for the vampire bat and Anubis; else two: the scorpion queen's egg clusters either side of her,
   Sobek in a long wade, Seth in a pool between two drifts, Babi between two one-cell death pits to leap, the Mother
   of Webs in the dark with two braziers, Apep in a pit between two ledges (the teleporter on one, the boss gate on
   the other, a ladder down from each).
10. **Content.** As on a test level, along the route and the wings, only single spikes; treasure at the end of 60% of
   the wings no key took. Pits only on dry floor, spikes never next to half water (no jump out of it), objects keep the water or sand of their cell. At most
   (4 + 2 × difficulty) × size / 100 monsters. The score aimed at is `targetScore(difficulty, size)`, the test
   level's per 50 open cells; the candidate closest to it and to the size wins.
11. **Supplies** (step 7), the campaign's rules ([Levels](#levels)) as generator rules. With a poisoner (plants too),
   a dart plate or a poison drip on the level: an antidote in the chest (or at the free end) of the wing nearest
   before the first hall with poison, a lesser resistance potion in a plain potion's chest, from the scorpion nest
   on a greater one too. A Blessing of Hapi in a chest 2 to 6 cells before the first half water the route wades
   through (or near the end of the hall before). Then the zone's trapped chests (`ZONE_TRAPPED_CHESTS`,
   `src/world/campaign.h`: none in the first two zones, two in the web halls where they come in, three from the land
   of Sokar on), each a chest that held a plain potion (health, stamina, might, armour). Weapon chests come in by the
   difficulty as on a test level (`WeaponDef::generatedFrom`).
12. **Carried content** (step 8; `GenOptions`, levelgen's `--texts`, `--page`, `--title`, `--stargate`). The
   inscriptions on the halls of the route, between where it comes in and leaves (on the way): the ids given, else 2
   or 3 (1 or 2 in the tomb entrance) of the zone's texts in `content/inscriptions/rebuild.txt` (`ZONE_TEXT_FIRST`,
   `src/world/level_gen.h`), at most one with a boon; the hints bound to one thing stay out (the ring transporter's,
   the last fork's, the Stargate's), the name gate's comes only with a name gate. Without the texts (a unit test's
   level, levelgen run off the repo root) none. Each secret room gets a lore text (`SECRET_ROOM_TEXTS`, no second
   boon). An expedition page lies on a skeleton at a free wing's end, the title goes in the level file. The
   Stargate chamber's stand-in (`--stargate`, for one level of the campaign until
   [stargate-chamber](plan/stargate-chamber.draft.opus.md) is built): the level's name gate, its password the king's
   name, leads to a room with the ring's text (`STARGATE_TEXT`) and a rare chest.
   A route fork (`GenOptions::fork`, `--fork 1`, 2 for the last fork; on a level going down): a hall the route
   crosses leftwards is split by a pillar of rock under the link into it, so the hall over it gets two drops side by
   side, one each side of the pillar. The left route crosses the long left part and drops into the hall below where
   the route did, two more monsters on it; the right one, short, drops a few cells along, traps on every other cell,
   at its end the earlier expedition's remains (a skeleton) and their gear (a weapon of the depth or a rare chest).
   Both drops are one-way: no way leads from one route to the other, and the checker's softlock rule proves each
   reaches the exit. On the last fork the warning (`FORK_TEXT`) stands between the drops; a page given lies on the
   way to the fork, on the floor. The player reaches one drop first and jumps it to take the other.
13. **Decor** (step 9). The game scatters the rest by the zone's look (`ZONE_LOOKS`, `src/world/zone_look.h`); the
   generator places only what tells something in the decor layer: a sarcophagus (one), canopic jars, pottery and
   scrolls on the free floor of the treasure tombs, secret and hidden rooms and the fork's tomb; before the boss
   gate the zone's guardian statue (`ZONE_GUARDIAN_PROP`: a cat in the web halls, a jackal in the priests' quarter,
   Seth's desert and the Duat, Osiris in between, none in the caves) and a winged sun on the wall beside it; webs
   under the ceiling of the web halls' dark halls. The braziers, the dark halls' torches and the figures round the
   mummies and ushabti came with steps 2 and 3.

Still missing (the next steps of the plan): the zone's traps, the riddle and name gates, the boss level, the supplies
by the rules above, the carried content and the decor.

## Test levels from ASCII

`tools/level/ascii2level.py IN.txt OUT` builds a level file from a drawing in the `levelcheck --map` legend. It
reads the legend from `levelcheck --legend`, which prints the game's tables (`src/world/tile_defs.cpp`,
`monster_kinds.cpp`, the lock colours in `level.h`), so the drawing, the checker and the game cannot disagree.
In `--map` output, `m`, `q` and `Q` mark a monster type, key colour or gate colour the game does not know.
A `def CHAR TYPE ATTR VALUE` line adds a character to the legend for that file, for example `def L 12 2 0`
(blue lever) or `def 1 8 3 1` (large health potion).
The drawing is the object layer. A line `structure` can follow it with a second drawing of the same rows, the
structure: `#` wall, `.` empty, `~` half water, `=` deep water. Then `#` and `.` in the first drawing just mean no
object. Without it, the structure follows from the object drawing: `#` wall, anything else empty. A line `wall` can
follow (after the structure drawing, if any) with the wall drawing, the same rows: the fixtures (`/` lever, `i`
inscription, `|` wall spikes, `+` fire vent, `,` poison drip, or a `def` of a wall type), `#` and `.` for none. A wall glyph in the object drawing goes
to the wall layer too, so a fixture alone in its cell needs no wall drawing. `levelcheck --map` shows a cell's fixture
where it has no object, prints the structure drawing after the map when the level has water, then the wall drawing
when it has fixtures. `levelcheck --legend` marks the wall types (`wall:`). A level with placed props gets a `decor`
drawing (`a` + the prop's index, `-` bare), one with placed decals or torches a `decals` drawing (`a` + the decal's
index, upper case with a torch, `t` a torch alone, `-` bare or no torch); `--legend` ends with their `prop` and `decal`
lines. The ASCII sources have no decor or decal drawings: the editor paints them.
Example with both layers in a cell: `tests/levels/wall_layer.txt`.
Examples: `tests/levels/mechanisms.txt`, `tests/levels/rats.txt`, `tests/levels/bats.txt` (the built files sit next to them).

## Tests

- `tests/unit/level_gen_test.cpp` (plan campaign-levelgen): zone levels over two seeds per zone pass the checker
  with no warnings, fill the grid, keep the target size and name their zone; between them tall halls, pillar loops,
  hubs, ledges, shafts and gates turn up; each zone's terrain is the one of the zone table (water, deep water, sand,
  quicksand, sand falls, dark halls, torches, braziers, light shafts, sun discs turned on the path) and its monsters
  come from its set (camel spiders mostly in the dark, worms in the sand, decoy coffins, decor ushabti); the same seed
  gives the same level; the `gen:` specs. Scenario
  `tests/scenarios/campaign_levelgen.txt`: one zone level per zone (screenshots).
- `tests/unit/sun_mirrors_test.cpp` (plan sunlight-mirrors): the beam's way through a mirror (turned, edge-on,
  catching it), the search over the mirrors the player can turn, the checker (the turn on the path, a mirror out of
  reach, a disc lit at the start, the warnings), the mirror turned in the game lighting the disc and opening its gate,
  a mummy burning in a beam (`tests/levels/mirrors`, `mirrors_mummy`). Scenario `tests/scenarios/solved/sun_mirrors.txt`:
  the beam, the turned mirror, the lit disc (screenshots).
- `tests/unit/sand_test.cpp` (plan desert-chapter): sand slows and stops the sprint, quicksand sinks, hurts and a
  jump frees, the sand fall hits, slows and leaves a drift, a sand worm lies under its sand and comes up under the
  player (`tests/levels/sand`, `tests/levels/sand_worm`), and the campaign's 35 levels with the desert at 21-25.
  Scenarios `tests/scenarios/solved/sand.txt`, `sand_worm.txt`, `desert.txt`: the drifts, the pour, the sunk player, the
  ripple, the sandstone halls and Seth's oasis (screenshots).
- `tests/scenarios/solved/mechanisms.txt`: key, gates, lever, rock falls (hit and dodged), keys kept over save and load.
- `tests/unit/world_rules_test.cpp` (on the sim harness, `tests/unit/sim_world.h`): a rock fall walked through,
  grazed, stepped back from, a direct hit; a dart trap's volley outruns the walk and poisons, a running jump over the
  plate sets nothing off, a heavy monster (the Anubis) presses it and takes the darts; the fall trap; an arrow into
  half water; a teleporter pair and the exit only it reaches; leaving a level mid-jump; the boss room (minions on
  arrival and up to the limit, 1 XP each while the boss lives, the sealed boss gate, no boss after a load); lvl5
  played through; the Anubis boss's coffins, blows and gold key, his coffins after a load; a generated level and
  campaign levels 6 and 15 load.
- `tests/scenarios/solved/rats.txt`: rat and giant rat screenshots (size, attack, die).
- `tests/unit/monster_rules_test.cpp` and `tests/unit/sim_test.cpp` (on the sim harness): walkers stop at a wall,
  floor spikes and a pit; a rat stops at an armed rock fall, a giant rat leaps over it, neither sets it off; a reckless
  monster (the mummy, the Anubis) walks into spikes and a rock fall and takes its share of the damage (`Courage`,
  `trapDamagePct`), a trap's kill gives no XP; the giant rat leaps spikes and a pit (2 s apart), not a 3-cell gap, and
  outruns the player; the giant scarab leaps and bites; monster poison, damage types, the venom amulet; health bars
  stay hidden until a monster chases, bites, swoops or is hit; the cobra's spit, Sobek's charge, Apep's dive, the
  bat's swoop.
- `tests/unit/scen_u_test.cpp` ("spikes: a hit every 100 ms ..."): spike damage rate and ramp, the hitbox edge, paused in the inventory, two tiles.
- `tests/unit/reach_test.cpp`: scarab, rat, worm, giant scarab, boss scarab and giant rat walk up and stop where they
  bite; every melee weapon hits them there, also in toon mode; an arrow hits a giant rat 2 tiles away
  (`tests/levels/hitbox_*`).
- `tests/scenarios/solved/scarabs.txt`: scarab and giant scarab screenshots (size, texture).
- `tests/scenarios/solved/mimic.txt`: a mimic next to a real chest: looks like the chest, shows "[E] Open" in its cell,
  wakes on the interact key, bites, leaves a real chest when killed.
- `tests/unit/interact_test.cpp`: the interact verb of every usable cell (`tests/levels/interact`), the key doing
  something exactly when there is one, a mimic waking on the key or a hit and not on coming near, the inscription
  glint. `tests/scenarios/solved/interact_prompt.txt`: the prompt and the glint, screenshots.
- `tests/unit/autosave_test.cpp`: the exit right by the entrance (`tests/levels/autosave`) asks for the autosave at
  the next level's start. `tests/levels/autosave_play`: a riddle gate and two rats, for `tests/play/autosave.txt`.
- `tests/scenarios/solved/anubis_bolt.txt`: the Anubis walks up and shoots its red bolt from afar, then hits up close.
- `tests/scenarios/solved/mummy.txt`: a mummy in its coffin: no health bar, wakes 1.6 tiles away, climbs out (rise clip),
  strikes, dies; the empty coffin stays.
- `tests/scenarios/solved/bats.txt`: bat and giant bat screenshots (roosting, swoops through the player, kill, fall).
- `tests/scenarios/solved/bow.txt`: the bow draws and shoots; an arrow with nothing in reach lands on the floor, one in reach is
  aimed at the plant (`tests/levels/archery`) and hits it, the shots kill it.
- `tests/scenarios/solved/ranged_weapons.txt`: composite bow, sling, throwing stick and javelin each hit the archery plant
  from inside their range; screenshots of the draw, the whirl and the missiles in flight (the stick flying back).
- `tests/scenarios/weapon_reach.txt`: every melee weapon at its hit frame next to a giant rat at bite distance, with the
  debug boxes: the reach should end near the weapon's tip.
- `tests/unit/scen_u_test.cpp` ("moving or sprinting does not cancel an attack recovery"): sprinting through a recovery does not cut it short.
- `tests/scenarios/solved/weapons_held.txt`: every weapon in the fist, standing and at its strike, both facings: screenshots.
  The swing's poses: `tests/unit/swing_test.cpp`.
- `tests/scenarios/solved/vampire.txt`: the vampire bat's roost (`tests/levels/vampire`): its bats, its bites heal it
  (`hurtboss`), more bats up to the limit, its death opens the boss gate to the gold key.
- `tests/scenarios/solved/summon_effects.txt`: scarabs digging out in a spray of sand, bats dropping from the ceiling,
  mummies out of the Anubis boss's coffins (`tests/levels/summon_dig`, `summon_drop`, `summon_coffin`): screenshots.
- `tests/levels/classic1`, `classic2`: the old hand-made levels 1 and 2, kept as fixtures for the scenarios that
  depend on their layout (ladders, chests, the draft map, ...), so the campaign levels can change.
- `tests/unit/scen_u_test.cpp` ("a generated level (seed 81, difficulty 8) plays from start to exit"): plays `tests/levels/gen_d8` (seed 81, difficulty 8) from entrance to exit.
  Written by `levelcheck --script`.
- `tests/unit/level_test.cpp`, `wall_layer_test.cpp`, `wall_spikes_test.cpp`, `water_test.cpp`, `climb_test.cpp`: the
  level file (v1 and v2 converted, save and load), the wall layer, wall spikes and water in the game and the checker,
  climbers on ladders. `tests/unit/tile_defs_test.cpp`: the legend's glyphs.
