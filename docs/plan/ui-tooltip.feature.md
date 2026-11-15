# UI tooltips: hover text on the shared UI screens

Status: implemented 2026-11-15, waiting for the user's play-test. Was: draft 2026-11-15, size M, needs nothing first.

## Idea

The UI screens have no tooltip, only the screen-switch tabs' name labels (plan screen-switch-tabs) and the journal's
hovered ribbon label. A shared tooltip lets a screen show a short total and explain it on hover (first use: the armor
split's `cut: 20% + 10a`, plan [armor-split](armor-split.feature.md)).

## Plan

* One shared part in the UI kit ([ui.md](../ui.md)): `ui::tooltip(anchorRect, text)` queued during the screen's draw
  and drawn last, on top of everything. A dark label like the tab names: `PANEL`-style fill, `BRONZE` frame,
  `TEXT` font, one or more lines.
* It shows after a short hover delay (~300 ms) and hides on mouse-out, a click or a key. It is placed under the
  anchor, flipped above or sideways to stay inside the canvas.
* Keyboard / gamepad: the focused element's tooltip shows too, if the screen has focus.
* Move the screen-switch tabs' name labels onto it, so there's one look.
* Document it in [ui.md](../ui.md) (parts list) with a screenshot scenario.

## Play-test

* `bin/game --play tests/play/ui-tooltip.txt` (opens the inventory): hover a screen-switch tab: its name shows in
  the new tooltip, after a short delay, inside the screen near the edges.
