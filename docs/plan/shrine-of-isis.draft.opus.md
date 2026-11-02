# Shrine of Isis: lift curses, then ward them off

Status: draft 2026-11-02, needs nothing first. From the user.

## Idea (from the user)

A Shrine of Isis in the dungeon. Visit it (interact key, E) and every curse on the player is lifted, and the player
gets a protection from curses for 2 minutes: the Blessing of Isis.

## Plan

* New tile/object: the shrine. Pick a free level glyph (AGENTS.md, Glyphs). Procedural Blender 5 model
  ([modeling.md](../modeling.md)): a small Isis shrine (naos with a winged Isis relief or a seated Isis statue,
  offering table). Add to the editor tile info and the glossary.
* Interact verb "Pray" (`Interaction`, `Dungeon::InteractVerb`, interact prompt). On use:
  * clear every active curse (`Curses`), greater ones too;
  * start the ward: 2 minutes in which no curse can land, from any source (monster melee/ranged curse entries, tomb
    curses, trapped chests, cursed coffins). A blocked curse shows a short message instead of "A curse!".
  * Ward ends on death, like the Blessing of Hapi potion.
  * HUD: shown as a card like the curse tiles (same size, layout, timer), but with a yellow background instead of the
    curses' purple frame (from the user).
  * Message + sound (a soft chime / sistrum shake; `tools/audio` style).
* Uses: one per shrine (it dims after use), so a shrine is a placed resource, not a camp. Save its used state.
* Placement: a few in the campaign, in levels with heavy curse sources (cursed mimics, mummy priests, tomb-curse
  zones), before a hard fight or deep in a side branch. Every touched level passes `bin/levelcheck` with no warnings;
  the checker may treat a shrine as a curse counter where it weighs curse threat.
* Journal field note on the first use (`FieldNote`, `journal_text.cpp`).
* Tests: unit test (curses cleared, ward blocks a curse, ward expires after 2 min, ends on death, one use);
  play script `tests/play/shrine-of-isis.txt` (curse the player, stand by a shrine).

## Open points

* Name clash: "Blessing of Isis" is already a skill (`SkillId::BlessingOfIsis`, curses last half as long). Either name
  the shrine ward differently (e.g. "Protection of Isis", "Ward of Isis") or rename the skill. Ask the user.
* Should the skill interact with the shrine (e.g. ward lasts 4 min with the skill)? Implementer's choice; mirrors
  Favour of Hapi lengthening the Blessing of Hapi.
* One use per shrine vs reusable with a cooldown: plan picks one use.
* Does the ward also block insanity (the cursed mimic's only special)? Plan: yes, it is a curse.
