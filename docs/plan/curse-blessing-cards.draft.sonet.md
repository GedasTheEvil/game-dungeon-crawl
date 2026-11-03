# Status cards: one painted background per kind, the icon on top

Status: draft 2026-11-03, needs [shrine-of-isis](shrine-of-isis.md) first (its yellow Isis card is one
of the backgrounds). From the user.

## Idea (from the user)

The HUD's status tiles (curses, blessings, poison, daze) are drawn in code now, each its own way. Paint them to a
texture instead. Each tile has two parts: a background and an icon on top of it. The background says the kind at a
glance: every curse purple (as now), every blessing yellow (as the Protection of Isis), every other status effect
(poisoned, burning, dazed by the bat) a neutral grey. One consistent system, easy to extend: a new blessing, curse or
condition later is one icon and one line, no new draw code.

## Now

`src/ui/player_hud.cpp`, one draw function per effect, each with its own look:

* `drawPoison`: stone tile, code-drawn drop (`drop`) per tier colour, 1..3 pips.
* `drawResist`: stone tile, code-drawn blue drop, 1..2 pips.
* `drawWaterBlessing` (Blessing of Hapi): stone tile, code-drawn blue-green drop.
* `drawIsisWard`: flat yellow card (`fillRect`), the throne icon (`Icon::IsisWard`).
* `drawCurses`: purple gradient frame, stone tile, the curse icon (`Icon::FirstCurse + c`), 1..2 pips.
* `drawDaze`: sand-gold frame, stone tile, the stars icon (`Icon::Daze`).
* `drawPoisonText`: the seconds left, repeating the same order by hand. `potionTiles`, `curseTiles`: the slot counts,
  again by hand.
* Burning (`Player::burning`) has no HUD tile at all.

## Plan

### The cards

`enum class StatusKind { Blessing, Condition, Curse }` (`player_hud.h`):

The kind follows what the effect does to the player:

* Blessing, yellow-gold (the Isis card's `ISIS_TOP/BOTTOM`): any good effect, whatever its source. Blessing of Hapi,
  Protection of Isis, the resistance potion, future blessings and good potion effects.
* Condition, neutral grey stone: a bad effect that is easy to end: an antidote cures poison, water puts out fire, a
  daze wears off fast. Poison (each tier), burning, daze.
* Curse, purple (`CURSE_FRAME_TOP/BOTTOM`): a bad effect that is almost incurable (only a prayer at a Shrine of Isis
  lifts it). The 8 curses.

New generator `tools/textures/status_cards.py` (PIL, the `hud_icons.py` style: drawn at 4x, downsampled, `bleed`)
writes `media/textures/ui/status_cards.png`: one row, one 128 px cell per `StatusKind`, in enum order. Same shape and
frame width on all three; a bevelled frame, a face with a little grain, the middle calm (the icon goes there), the
bottom strip clear for the pips. The three must tell apart at a glance at HUD size, also for colour-weak players
(the frames' brightness differs too, not only the hue). Load it beside `hudIcons` (`Texture statusCards`,
`texture_registry.h`, `assets.cpp`).

### The icons

All from `hud_icons.png`, painted. New cells (free: 5..7, 45..47):

* `Icon::Drop`: a white drop, tinted by the HUD like the potion flask: poison tiers (`POISON_DROPS`), resistance
  (`RESIST_DROPS`), Hapi (`WATER_DROP`). Replaces the code-drawn `drop` on status tiles.
* `Icon::Burning`: a flame.

The rest exist: `IsisWard`, `Daze`, the curses.

### One list, one loop

* `PlayerHud::View` gets the running effects as one list in display order: `struct Status { StatusKind kind; Icon icon;
  Color tint; int pips; int leftMs; }`, a fixed-size array plus a count (no allocation per frame). Built in
  `player_hud_view.cpp` from `PlayerStats` and `Player::burning`.
* Order: blessings, then conditions, then curses; inside a kind as now (resistance, Hapi, Isis; poison weak first, burning, daze; curses in
  `Curse` order).
  Same kinds sit together, so the row reads by colour.
* One `drawStatuses` loop: drop shadow, the kind's card, the icon (tinted), the pips; one loop for the seconds. Remove
  `drawPoison`, `drawResist`, `drawWaterBlessing`, `drawIsisWard`, `drawCurses`, `drawDaze`, `potionTiles`,
  `curseTiles`, the per-effect `View` fields the list replaces, and the colour constants left unused.
* A future effect: a cell in `hud_icons.py`, an `Icon` entry, one line in the view builder.
* Burning: the time left of `Player::burning` (it ends on wading or douse); no pips.

### Docs and tests

* `docs/ui.md` (HUD), the comment block in `player_hud.h`, the glossary rows "curse" ("Purple-framed HUD tiles") and
  "Protection of Isis". New glossary rows: "status card" (the three kinds, their colours and the rule: good, bad but easy to end, bad and
  almost incurable) and the kind names.
* `docs/development.md` (Where is X, or how to add an effect): adding a status effect to the HUD.
* Unit test: the view builder orders a mixed set (resistance, poison, burning, a greater curse) by kind, with the right
  pips and icons.
* Scenario in `tests/scenarios/`: Isis, Hapi and resistance, poison tier 2, burning, daze, two curses (one greater) running; one
  screenshot of the HUD row. Check the three cards tell apart, the icons sit centred, the pips read on all three.

## Open points (implementer's choice)

* Card corners: square as now, or slightly rounded. Keep the size so `poisonRect` and the layout stay.
* The row may grow long with many effects at once (now up to ~15). Not in scope; note it if the screenshot shows it
  running into something.
