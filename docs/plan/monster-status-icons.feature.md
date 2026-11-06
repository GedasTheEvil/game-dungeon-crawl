# Monster status icons: small effect icons over a monster's health bar

Status: implemented 2026-11-06, waiting for the user's play-test. Was: draft 2026-11-06, size M, needs nothing first.

## Idea

From the user: the player sees their own effects as status cards over the HUD (`statusList`, `status_box.cpp`,
[curse-blessing-cards](solved/curse-blessing-cards.md)), but a monster's effects only show as body effects (stars for
a stun, flames, maybe nothing for poison). Give a monster the same language: small icons in a row just above its
health bar, the same icons and kind colours as the player's cards, so "poisoned", "burning", "stunned" read the same
on both. Part of the user's "the player is a monster too" idea ([same-hazards-for-all](same-hazards-for-all.feature.draft.opus.md)),
but it does not need it: it reads today's `Monster` getters.

## Plan

* **What shows** (in this order, the player's card order):
  * Poison, one icon per running tier (`Monster::PoisonMask`), the `Drop` icon tinted as the player's tier card,
    with its pips.
  * Burning (`Monster::Burns`), the `Burning` icon.
  * Stunned (`Monster::Reeling`), the `Daze` icon (the same stars as the player's daze).
  * Terrain mood, once [terrain-affinity](solved/terrain-affinity.feature.md) is in (it is being built): native
    as a Blessing-gold card, hostile as a Condition-grey card, the terrain's own small glyph if there is one in the
    atlas, else an up / down arrow. Skip it if terrain-affinity is not done yet; add it then.
  * No blessings or curses: monsters do not pray at shrines. A boss's own powers (life steal, ...) are not statuses.
* **Shared list**: split `statusList` (`src/level/status_list.cpp`) so the condition part (poison tiers, burning,
  daze / stun, terrain mood) is one function both the player's HUD and the monster's row call; the player's adds
  blessings and curses around it. One place to add the next condition.
* **Drawing** (`src/graphics/monster_draw.cpp`, where the health bar is drawn once a monster is alerted): the cards
  shrunk to a small square (1.5 times the health bar's height, the user's start value; tune later by play-test), the kind's
  background cell (`status_cards.png`) with the icon on top, left-aligned over the bar, no seconds text (too small);
  the pips only if they stay readable. Only while the bar shows (alerted, not a boss).
* **Bosses**: their bar is on the HUD (`boss_bar.cpp`): the same row of icons over that bar, at the player's card size.
* **Scenario test**: a monster poisoned and burning (a fire arrow, a poison dart trap) and one stunned; screenshots in
  `tests/out/monster-status-icons/`, check the icons sit over the bar, readable, not clipped at the screen edge.
* Glossary row: "status icons" (monster), linked to "status card".

## Open points (implementer's choice)

* Whether pips fit at that size.
* Whether the stun icon replaces the stars over the head or sits beside them (likely beside: the stars are the body,
  the icon the HUD language).

## As built

* `addConditions` (`src/level/status_list.cpp`) fills a body's conditions from its `Vitals` (plan shared-vitals) and
  its terrain mood; `statusList` (the player) and `monsterStatuses` (`Monster::Statuses`) both call it.
* `PlayerHud::drawStatusCard` draws one card at any size (its parts scaled from the HUD's); the HUD, the boss bar and
  the monster row (`drawStatusRow`, `src/graphics/monster_draw.cpp`) use it. Pips only on the HUD and the boss bar:
  too small on a monster's row. The stun card sits beside the stars over the head.
* Terrain mood: two new atlas cells (`hud_icons.py`, the atlas now 8 x 7), an arrow up (green, on the gold card) and
  an arrow down (red, on the grey card).
* Scenario `tests/scenarios/monster-status-icons.txt`: the burning, stunned mummy; the Bat of Shezmu poisoned and
  stunned on its HUD bar, its bats' native arrows.

## Play-test

* `bin/game --play tests/play/monster-status-icons.txt` (the Bat of Shezmu's roost, mace with Stun, venom amulet; god
  mode): hit the bats and the boss.
* A small card row over each bat's bar: its native gold arrow up, the stars card while stunned, a green drop while
  poisoned; readable, not clipped at the screen edge.
* The boss's cards over its HUD bar at the player's card size, with pips for the poison tier; clear of its name.
