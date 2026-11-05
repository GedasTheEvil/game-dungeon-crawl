# Shrines of Montu, Ptah, Amun and Wadjet

Status: draft 2026-11-05, size L, needs nothing first. From the user.

## Idea (from the user)

More shrines, each to another god, built like the [Shrine of Isis](solved/shrine-of-isis.md). Pray (interact, E) for a
blessing that lasts 2 minutes:

* Shrine of Montu: Might +5.
* Shrine of Ptah: Armor +5.
* Shrine of Amun: heals fully, then max HP +20%.
* Shrine of Wadjet: cures poison, then the player resists poison fully (the user wrote "Wdjet"; the usual name is
  Wadjet, the cobra goddess).

The look comes from the Shrine of Isis. The user suggests splitting the model into the base shrine and the god's statue;
the implementer decides.

## Plan

* Model: split it in the Blender script, not in the game. `build_shrine_isis` (`tools/blender/models/decor.py`) becomes
  `naos(b, M, wall, emblem)` (plinth, walls, cornice, back wall in the god's colour, offering table) plus one statue
  function per god (`isis_statue` is already one). Export one model per shrine, so the game, the editor and the
  checker see one prop each, the same as the Isis shrine. A runtime split (one naos plus a statue part) would need a
  new two-part prop for no gain. Statues: Montu falcon-headed, a sun disc with two plumes, a khopesh; Ptah mummiform
  in a skullcap, holding the was-djed sceptre; Amun with the tall double plume, seated like Isis; Wadjet a rearing
  cobra in the red crown. Wall emblems in place of Isis's wings: Montu sun disc, Ptah djed pillar, Amun ram, Wadjet
  papyrus stalks. Steps: [modeling.md](../modeling.md).
* Props: `DECOR_SHRINE_MONTU`, `_PTAH`, `_AMUN`, `_WADJET` (`src/world/decor.h`), a free glyph each (AGENTS.md, Glyphs),
  editor tile info and icons (`tools/editor/`), verb "Pray" (`Interaction::Pray`, `dungeon_interact.cpp`).
* Blessings: timed `PlayerStats` effects like `IsisWarded`, 2 minutes. They end on death, and a prayer at the same god
  starts the time again. Different gods stack (Montu and Ptah at once is fine). Nothing is saved, the same as the Isis
  ward.
  * Montu: `CurrentMight()` +5. Ptah: `CurrentArmor()` +5. For scale: the grand Amulet of Strength gives +6, and a
    player level gives +1.
  * Amun: HP to max, then max HP +20% for 2 minutes. When it ends, HP is clamped to the normal max and never drops
    below 1.
  * Wadjet: ends poison, and no poison lands for 2 minutes (the HUD shows it as a ward). Same block message as the
    Isis ward ("Wadjet turns the venom away").
* Uses, the implementer's call: **one prayer per shrine per level** (recommended). Unlimited Amun heals turn a shrine
  into a free healing well, and unlimited stat buffs mean praying before every fight. After a prayer the shrine is
  spent: its lamp goes out or its offerings are gone, and the prompt says "The god is silent". Save which shrines are
  spent in the level state. Isis stays unlimited.
* HUD: yellow blessing status cards (`status_list.cpp`, `PlayerHud::Icon`). The icons come from `tools/textures/hud_icons.py`:
  falcon/khopesh, djed, ankh or ram, cobra.
* Sound: the Isis chime with a different pitch or instrument for each god (`tools/audio/shrine_sounds.py`).
* Light: the prayer's beam (`SunBeam::ISIS_LIGHT`) in each god's tint: Montu red-gold, Ptah green, Amun deep blue,
  Wadjet green-gold.
* Placement: rare, like Isis, about 3 to 5 levels per god, never two shrines in one room. Put each where it fits: Montu
  and Ptah before a boss or hard fight, Amun deep in a side branch after a heavy fight, Wadjet on levels with
  poisoners (spiders, scorpions, cobras). Every touched level passes `bin/levelcheck` with no warnings. The checker
  should not count on the blessings: they are optional, and the player may already have used the shrine.
* Glossary rows: each shrine and each blessing. Journal: a field note on the first prayer at each god.
* Tests: a unit test for each blessing (applied, ends after 2 minutes, ends on death, the same god restarts it, two
  gods stack, Amun's HP clamp, Wadjet blocks poison), plus the one-use rule if it is chosen. A play script with all
  four shrines in reach.

## Open points

* One use per shrine (recommended) or unlimited like Isis.
* Should a skill lengthen the blessings, as Blessing of Isis lengthens the Isis ward? Not needed now.

## Play-test

* `bin/game --play tests/play/shrines-of-the-gods.txt`: pray at each shrine. The card shows, and Might, Armor, HP or
  poison changes as promised. After 2 minutes the card is gone and the stat is back.
* Look at the four shrines: each god can be told apart at a glance and matches the Isis shrine's style.
