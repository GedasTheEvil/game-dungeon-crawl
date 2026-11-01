# Tomb-curse containers: mimic jar, coffins

Status: implemented 2026-10-31, waiting for a play-test (`bin/game --play tests/play/tomb-curse-containers.txt`, or
`bin/game --level 15`: a mimic jar at col 31 row 16; coffins stand among the containers from level 10 on). Draft
2026-10-30, from the project audit of 2026-10-30.

Opus: new container kinds and maybe a model. Left from [treasure-containers](solved/treasure-containers.md) and
[trapped-chests](solved/trapped-chests.md).

* A mimic jar: a canopic jar or sealed pot that is a mimic, smaller and weaker than the chest mimic, in tomb levels.
  Its own bite and wake sounds.
* Sarcophagi / coffins as tomb-curse objects: opening a coffin (not a mummy coffin that wakes) gives loot with a
  chance of a curse (`src/world/curses.cpp`), with a tell for the attentive (an inscription on the lid).
* Add to `src/world/containers.h`, loot rolls, the journal (field note), levelgen depth rules; place a few by hand.

## Done (2026-10-31)

* **Coffin** (`Container::Coffin`, `containers.h`): a new container kind, an anthropoid coffin of varnished wood
  lying on the floor (wig, gilt face, collar, crossed arms, a column of text down the legs). From the tombs on:
  7 % of the containers in the tomb tier, 8 % in the temple tier (taken from the coffers, baskets and canopic jars;
  chests stay at 40 %, jars at 10 %). Too long to stand side-on (`ContainerDef::lengthwise`): only the stances facing
  the camera. Its clip: the lid rattles, lifts, is pushed back off and lies behind the case (`WorldSound::LidCoffin`).
  Not the decor coffins: those stay props (the mummies' and the decoys).
* **The curse**: 40 % of the coffins carry an execration text (`TreasureLook::inscribed`, `COFFIN_INSCRIBED_PERCENT`),
  rolled with the look (a hash of the level and the cell, so it is the same on every load). Opening one: the trap
  ward's chance stops it, else a lesser chest curse (weakness, clumsiness, vulnerability, fatigue or slowness) with the
  line "An execration text on the lid..." (`Dungeon::coffinCurse`). The risk pays: it holds a trapped chest's loot
  (the item and one bonus more). A trapped coffin (`TrappedTreasure`) curses through its trap (`RollChestTrap`: always
  a curse), no second roll. One roll per object, as [trapped-chests](solved/trapped-chests.md)'s rule.
* **The tell**: the text is red, on the lid's column and round the case under the lid (the side the player sees),
  black on red where a plain coffin has blue and black on varnish. Drawn as a second texture over the coffin
  (`container_coffin_cursed.png`), faded in by the player's distance like the trapped chest's tell (`coffinTell`):
  never from afar. Trapped coffins show the text instead of the darker shade.
* **Mimic jar** (`MonsterMimicJar` = 35, glyph `}`, "Jar of Set"): the sealed pottery jar container (the same mesh
  and scale) until opened or hit, then it bites where it stands (`Locomotion::Ambush`, like the mimic). 380 HP,
  45 / 800 ms, 1100 XP, threat 1.5: under its home zone's class target (`bin/balance home "mimic jar"`: 11 % HP lost in
  5 s against the mimic class's 20 % in 6 s). Killed, it leaves its smashed jar (`MonsterKind::disguise`,
  `ContainerCell::droppedKind`, saved as `j` / `k`) with a potion (`RollMimicJarLoot`). Its own wake, bite and death
  sounds.
* **Placement**: by hand in levels 15, 16, 18, 47 (two), 48 and 49, each a plain mimic turned into a mimic jar; every
  level passes `bin/levelcheck` with no warnings. Levelgen: `ZoneFauna::jarPercent` (the Priests' Quarter 40 %, the
  Necropolis 50 % of the plain mimics), and from difficulty 6 on 30 % of the other generated levels' plain mimics
  (`MIMIC_JAR_*`). Coffins are not placed by hand: a treasure's look is rolled, the level format has no field for it
  (as in treasure-containers).
* **Journal**: field note "Coffins" (`FieldNote::Coffins`, the first coffin opened: read the lid first); a cursed one
  also writes "Tomb curses". The mimic jar's bestiary note.
* Models: `tools/blender/models/containers.py` (`build_coffin`, the cursed texture baked on the same UVs),
  `tools/blender/models/mimic_jar.py` (no rig: every frame built whole from `containers.build_jar`, the fangs, throat
  and tongue in the neck, the stopper hinged as a lid; awake, attack, die and idle clips; the die clip ends in the
  container's smashed jar). Sounds: `tools/audio/tomb_curse_sounds.py`.
* Tests: `tests/unit/tomb_curse_test.cpp` (the coffin's shares and stances, the inscribed share over the campaign, the
  curse, the ward, the bonus, the trapped coffin, the tell, the mimic jar's lurk, wake, smashed jar and its save,
  levelgen), the scenario `tests/scenarios/tomb_curse.txt` (screenshots), level `tests/levels/tomb_curse`.

Left out: the canopic jar as a mimic (the sealed pot it is); boss rule: no boss has the mimic as its kin.
