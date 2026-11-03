# Shrine of Isis: lift curses, then ward them off

Status: solved 2026-11-03 (play-tested by the user). Was: draft 2026-11-02, needs nothing first. From the user.

## Idea (from the user)

A Shrine of Isis in the dungeon. Visit it (interact key, E) and every curse on the player is lifted, and the player
gets a protection from curses for 2 minutes: the Blessing of Isis (renamed to Protection of Isis, see Plan).

## Plan

* New tile/object: the shrine. Pick a free level glyph (AGENTS.md, Glyphs). Procedural Blender 5 model
  ([modeling.md](../../modeling.md)): a small Isis shrine (naos with a winged Isis relief or a seated Isis statue,
  offering table). Add to the editor tile info and the glossary.
* Interact verb "Pray" (`Interaction`, `Dungeon::InteractVerb`, interact prompt). On use:
  * clear every active curse (`Curses`), greater ones too;
  * start the ward, named "Protection of Isis" (from the user; "Blessing of Isis" is already a skill,
    `SkillId::BlessingOfIsis`): 2 minutes in which no curse can land, from any source (monster melee/ranged curse
    entries, tomb curses, trapped chests, cursed coffins), all kinds and grades, insanity too (from the user). A blocked curse
    shows a short message instead of "A curse!".
  * Ward ends on death, like the Blessing of Hapi potion.
  * HUD: shown as a card like the curse tiles (same size, layout, timer), but with a yellow background instead of the
    curses' purple frame (from the user).
  * Message + sound (a soft chime / sistrum shake; `tools/audio` style).
* Uses: unlimited, no cooldown (from the user). Praying again clears the new curses and restarts the 2 minutes.
  Nothing to save. Shrines stay rare instead.
* Placement: rare, about one in 4 levels (from the user): ~13 over the 54 campaign levels. Put them in levels with
  heavy curse sources (cursed mimics, mummy priests, tomb-curse zones), before a hard fight or deep in a side
  branch. Every touched level passes `bin/levelcheck` with no warnings; the checker may treat a shrine as a curse
  counter where it weighs curse threat.
* Glossary: add the shrine and Protection of Isis rows; the curse row's "no cure, no resistance" becomes the shrine
  rule (from the user: the new rules replace the old ones).
* Journal field note on the first use (`FieldNote`, `journal_text.cpp`).
* Tests: unit test (curses cleared, ward blocks a curse, ward expires after 2 min, ends on death, a second prayer clears and restarts);
  play script (curse the player, stand by a shrine), removed once play-tested; scenario `tests/scenarios/solved/shrine_of_isis.txt`.

## Open points

* Should the Blessing of Isis skill make the ward last longer (e.g. 4 min)? Implementer's choice; it would mirror
  Favour of Hapi lengthening the Blessing of Hapi.

## After the play-test

* A prayer also starts a light-blue beam of light on the player, the level up's sun beam in Isis's tint
  (`SunBeam::ISIS_LIGHT`, `PlayerStats::PrayedMs`, from the user).
