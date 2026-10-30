# Heka surplus: more skills than bosses

Status: On hold 2026-10-30: grill with the user first. From the project audit of 2026-10-30.

## Problem

Heka comes only from boss kills (`src/world/dungeon_boss.cpp`). The campaign has 11 bosses (lvl 4, 9, 13, 18, 22, 27,
32, 37, 41, 45, 54) and 9 skills (`SKILL_COUNT`, `src/world/skills.h`), none sealed. All 9 can be learnt after Rerek
(lvl41). Babi's heka buys nothing, and the Anubis heka comes when the game ends. `Heka::Earn` has no cap, so the HUD
heka tile pulses for the rest of the game (`src/ui/player_hud_view.cpp`) and the Heka screen shows "Heka to spend: 1"
with nothing to buy.

## Goal (from the user)

A defeated boss never gives something unusable. The design intent: more skills than bosses, so each run picks a
different set and feels unique.

## To grill

* How many new skills, which ones (Words of Thoth extras left out in [skill-hieroglyphs](solved/skill-hieroglyphs.md):
  riddle hints, secret doors, are candidates).
* Whether the last boss gives heka at all, or something else (the win).
* Meanwhile: no pulsing tile when nothing is left to learn (small, can go first).
