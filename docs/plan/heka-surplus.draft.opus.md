# Heka surplus: more skills than bosses

Status: draft 2026-10-30, needs nothing first. From the project audit of 2026-10-30, grilled with the user 2026-11-03.

## Problem

Heka comes only from boss kills (`src/world/dungeon_boss.cpp`). The campaign has 11 bosses (lvl 4, 9, 13, 18, 22, 27,
32, 37, 41, 45, 54) and 9 skills (`SKILL_COUNT`, `src/world/skills.h`), none sealed. All 9 can be learnt after Rerek
(lvl41). Babi's heka buys nothing, and the Anubis heka comes when the game ends. `Heka::Earn` has no cap, so the HUD
heka tile pulses for the rest of the game (`src/ui/player_hud_view.cpp`) and the Heka screen shows "Heka to spend: 1"
with nothing to buy.

## Goal (from the user)

A defeated boss never gives something unusable. More skills than heka, so each run picks a different set: a soft
goal of 4 to 6 skills still unlearnt at the end of the game, for a replay that tries other options.

The count: 10 heka to spend (lvl 4 .. 45 bosses, Anubis gives none), 9 + 6 new = 15 skills, 5 left over.

## Decisions (the user, 2026-11-03)

### Anubis gives no heka

The last boss (lvl54, the campaign's last level) gives no heka: the win is the reward. `Heka::Earn` refuses it, and
the boss-slain status line drops "You take its heka" there (the `else` branch already exists).

### Five stat skills

Flat, permanent stats, like the inscription boons ([inscription-boons](solved/inscription-boons.md)) but bigger. Names
and lore are the implementer's (gods matching the boons' themes: Montu for Might, Ptah for Armor, the ankh for life,
Shu for stamina); proposals below.

| Skill (proposed name) | Effect |
|---|---|
| Ka of Osiris | max health +25% |
| Strength of Montu | Might +5 |
| Djed of Ptah | Armor +5 |
| Breath of Shu | max stamina +25% |
| Swift feet (a god of speed: implementer's pick) | walk and sprint speed +20% |

* Applied where the stat is computed (`PlayerStats::MaxStamina()`, the max health, `Might`, `Armor`), not granted
  once: a skill is a state, so a load or a reset recomputes it. Learning the health or stamina skill fills the new
  share (current value scales with the max, or gains the difference: implementer's choice, no free heal beyond it).
* Speed: walking and sprinting only (not wading, not sand: they keep their own factors on top of the base speed,
  check that they multiply, not replace). **The jump is not affected**: its sideways reach stays today's
  (`WALKING_REACH`, `src/world/movement.h`), so the level checker and the polished levels stay valid. Cap the air
  speed at today's, or keep the take-off speed from the unskilled value. Trap timings that assume the walk speed
  (`rhythmRunClear`, `src/world/spike_cycle.h`) stay safe: faster only helps.

### Wisdom of Seshat (one exploration skill)

Seshat, goddess of writing and of measure (she stretched the cord for the temple plans), so she knows the hidden
rooms. One skill with all four parts (the user: each alone is worthless):

1. A riddle gate's hint shows at once (today after `HINT_AFTER_MISSES` misses, `src/ui/riddle.cpp`).
2. The level's royal name is known: the name gate's scroll gives it (journal entry as if the name cartouche were
   read, `Dungeon::LevelName`, plan secret-name-gates), so the name gate opens without the search.
3. Hidden rooms (`LevelBuilder::placeTeleporterRoom`) and secret rooms (behind a name gate) show on the map once the
   player is near (a few cells; implementer's radius).
4. Traps near the player are outlined faintly: spikes, death traps, rock falls, dart plates, wall spikes, fire vents,
   poison drips, quicksand. A soft glow or outline, visible in the dark, not the trap's full look.

Checker rule unchanged: nothing on the exit path needs the skill.

### Idle pulse guard

With 15 skills and 10 heka the campaign never runs out of skills, but test levels and the editor can: the HUD heka
tile pulses only while `Free() > 0` and some skill `CanLearn` (`view.hekaWaiting`, `src/ui/player_hud_view.cpp`).

## Heka screen

Today's look stays (the user): 2 columns, today's card size (`COLUMNS`, `ROWS`, `src/ui/skills_layout.h`). The
card list scrolls instead, with no limit on the skill count: the visible rows follow the selected card (arrows,
move keys), the mouse wheel scrolls, and a scroll hint (arrow or bar, the shared look of docs/ui.md) shows that
more cards lie above or below. Clicks map through the scroll offset. Checked with scenario screenshots at the top
and the bottom of the list. The new skills go at the end of `SkillId` (no save
mapping); a regroup by kind is the implementer's choice (old saves can be ignored). Six new icon cells in
`skill_icons.png` (`tools/textures/skill_icons.py`, `SKILL_ICON_*`). Card texts and lore in `content/texts/`.

## Also

* Glossary rows for the six skills, the skill names row, the heka row (Anubis gives none).
* Field notes for the new skills if the other skills have them (see [journal-field-notes-gaps](journal-field-notes-gaps.draft.sonet.md)).
* Tests: unit tests per stat skill (the stat with and without it, save/load), the jump reach unchanged with the
  speed skill, Anubis gives no heka; a scenario screenshot of the Heka screen and of the Seshat map and trap
  outlines.
