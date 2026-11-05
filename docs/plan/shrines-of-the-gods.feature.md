# Shrines of Montu, Ptah, Amun and Serket

Status: implemented 2026-11-05, waiting for the user's play-test. Was: draft 2026-11-05, size L, needs nothing first. From the user.

## Idea (from the user)

More shrines, each to another god, built like the [Shrine of Isis](solved/shrine-of-isis.md). Pray (interact, E) for a
blessing that lasts 2 minutes:

* Shrine of Montu: Might +5.
* Shrine of Ptah: Armor +5.
* Shrine of Amun: heals fully, then max HP +20%.
* Shrine of Serket: cures poison, then the player resists poison fully. The user first wrote "Wdjet" (Wadjet) and
  then asked: Serket, see Lore below.

The look comes from the Shrine of Isis. The user suggests splitting the model into the base shrine and the god's statue;
the implementer decides.

## Plan

* Model: split it in the Blender script, not in the game. `build_shrine_isis` (`tools/blender/models/decor.py`) becomes
  `naos(b, M, wall, emblem)` (plinth, walls, cornice, back wall in the god's colour, offering table) plus one statue
  function per god (`isis_statue` is already one). Export one model per shrine, so the game, the editor and the
  checker see one prop each, the same as the Isis shrine. A runtime split (one naos plus a statue part) would need a
  new two-part prop for no gain. Statues: Montu falcon-headed, a sun disc with two plumes, a khopesh; Ptah mummiform
  in a skullcap, holding the was-djed sceptre; Amun with the tall double plume, seated like Isis; Serket a woman with a
  scorpion on her head. Wall emblems in place of Isis's wings: Montu sun disc, Ptah djed pillar, Amun ram, Serket
  a scorpion. Steps: [modeling.md](../modeling.md).
* Props: `DECOR_SHRINE_MONTU`, `_PTAH`, `_AMUN`, `_SERKET` (`src/world/decor.h`), a free glyph each (AGENTS.md, Glyphs),
  editor tile info and icons (`tools/editor/`), verb "Pray" (`Interaction::Pray`, `dungeon_interact.cpp`).
* Blessings: timed `PlayerStats` effects like `IsisWarded`, 2 minutes. They end on death, and a prayer at the same god
  starts the time again. Different gods stack (Montu and Ptah at once is fine). Nothing is saved, the same as the Isis
  ward.
  * Montu: `CurrentMight()` +5. Ptah: `CurrentArmor()` +5. For scale: the grand Amulet of Strength gives +6, and a
    player level gives +1.
  * Amun: HP to max, then max HP +20% for 2 minutes. When it ends, HP is clamped to the normal max and never drops
    below 1.
  * Serket: ends poison, and no poison lands for 2 minutes (the HUD shows it as a ward). Same block message as the
    Isis ward ("Serket turns the venom away"). With the Serket's favour skill the blessing lasts twice as long, 4
    minutes, the same as Blessing of Isis does for the Isis ward (from the user). Update the skill's glossary row
    and its journal note (`note.serkets-favour`).
* Uses: every prayer gives the blessing, with no limit and no cooldown, the same as Isis (from the user). Nothing to
  save. Shrines stay rare instead (Placement).
* HUD: yellow blessing status cards (`status_list.cpp`, `PlayerHud::Icon`). The icons come from `tools/textures/hud_icons.py`:
  falcon/khopesh, djed, ankh or ram, scorpion.
* Sound: the Isis chime with a different pitch or instrument for each god (`tools/audio/shrine_sounds.py`).
* Light: the prayer's beam (`SunBeam::ISIS_LIGHT`) in each god's tint: Montu red-gold, Ptah green, Amun deep blue,
  Serket amber.
* Placement: at most one shrine of any god per level, Isis included (from the user). This is a soft rule for level
  design, so a level that needs two may have them; the checker does not warn. So a level that already has a Shrine of
  Isis gets no new shrine. Rare, like Isis, about 3 to 5 levels per god. Put each where it fits: Montu
  and Ptah before a boss or hard fight, Amun deep in a side branch after a heavy fight, Serket on levels with
  poisoners (spiders, scorpions, cobras). Every touched level passes `bin/levelcheck` with no warnings. The checker
  should not count on the blessings: they are optional, and they run out on the walk to the fight.
* Glossary rows: each shrine and each blessing. Journal: a field note on the first prayer at each god.
* Tests: a unit test for each blessing (applied, ends after 2 minutes, ends on death, the same god restarts it, two
  gods stack, Amun's HP clamp, Serket blocks poison, Serket's favour makes it 4 minutes), and a second prayer
  that gives the blessing again. A play script with all four shrines in reach (a test level may break the soft rule).

## Lore (why Serket, not Wadjet)

* Serket (Serqet) is the scorpion goddess who heals stings and snakebites. Her priests, the "kherep Serqet", were the
  physicians for venom. Wadjet is the cobra on the king's brow (the uraeus), a guardian of Lower Egypt and the king,
  not a healer.
* The game already gives Wadjet the poison items: the Ward of Wadjet potions (the greater one cures and resists poison
  95% for 2 minutes) and Wadjet's cobra amulet (the player's hits poison). A Shrine of Wadjet would repeat the greater
  ward.
* Serket already has the Serket's favour skill, Serket's scorpion amulet and the Gate of Serket zone. Shrine and skill
  pair like Isis: the Serket's favour skill doubles her blessing, the same as the
  Blessing of Isis skill.

## Implemented

* `God`, `GODS`, `shrineGod` (`src/world/shrines.h`); one timer per god in `PlayerStats` (`Pray(God)`,
  `Blessed`, `AdvanceBlessings`, `EndBlessings`); Isis's ward is the first of them.
* Blessing names: Fury of Montu, Shield of Ptah, Grace of Amun, Shelter of Serket (the skills already have Strength
  of Montu, Djed of Ptah).
* Amun heals to the raised max (a full heal at the new max, not the old).
* Saved, unlike the draft said: the Isis ward is saved too (`ISIS` line), the others in a `GODS` line.
* Placed: Montu lvl13, 27, 37, 53; Ptah lvl11, 17, 35, 44; Amun lvl7, 16, 31, 48; Serket lvl21, 25, 34, 43
  ([levels.md](../levels.md)).
* Decor drawing glyphs `M` `P` `A` `S` (`decorGlyph`): `'a' + index` ran past `~`.

## Play-test

* `bin/game --play tests/play/shrines-of-the-gods.txt`: pray at each shrine. The card shows, and Might, Armor, HP or
  poison changes as promised. After 2 minutes the card is gone and the stat is back.
* Look at the four shrines: each god can be told apart at a glance and matches the Isis shrine's style.
