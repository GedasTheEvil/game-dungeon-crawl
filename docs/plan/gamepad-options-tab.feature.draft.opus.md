# Gamepad tab in Options: a drawing of the pad with its bindings

Status: draft 2026-11-27, size M, needs nothing first.

## Idea

The user: Options is cluttered with text. After [sdl3-gamepad](sdl3-gamepad.feature.md), Controls has a 4th "Pad"
column and two lines of fixed keys under the table, and the Look tab mixes the mouse and the sticks. The pad gets a
tab of its own that shows a drawing of the controller, each button's action on a label at the end of a line from
the button (the user's reference: a pad outline with callout lines,
https://i.pinimg.com/736x/7b/84/29/7b8429bf1a5e4de6fe70ec6df30ff188.jpg).

## Plan

* A new Options tab "Gamepad" (`TABS` in `src/ui/menu/menu_internal.h`; six tabs then, `TAB_W` shrinks to fit
  `OPTIONS_PANEL`).
* Controls goes back to keys and mouse: drop the Pad column (`BINDING_SLOTS` stays 4 in the data, the Controls table
  draws slots 0-2 only, cells back to their old width) and the "Pad: ..." fixed line.
* The Gamepad tab: a flat drawing of a pad in the middle, drawn with the UI shapes (`ui_draw.h`: lines, ellipses,
  rects; the Egyptian palette, gold outline on the dark panel, as docs/ui.md), no texture: the body, the two grips, the
  sticks, the d-pad, the four face buttons, the shoulders and triggers on top, Back / Start. The face buttons and the
  caps by the family of the last used pad (`GameState::padFamily`: ✕ ○ □ △ in their colours, or A B X Y).
* Each bindable button has a label left or right of the drawing (left half's buttons left, right half's right), a
  thin gold line from the button to its label: the action bound to it (`Bindings::ActionFor(InputKey::Pad(b))`,
  `bindActionLabel`), "-" when none. The fixed parts labelled too, dimmer: left stick "Walk, climb", right stick
  "Look", Start "Menu".
* A label is a target (mouse, keyboard focus, d-pad) and works like an Options choice row: a click or Right steps
  the button to the next action, Left to the one before, through "-" (none). The action's old button is freed
  (`Bindings::Bind` moves it). Delete clears it. This changes a binding from the button's side, so the tab needs no
  capture mode.
* The stick rows (look speed, dead zone, invert Y) move from Look to under the drawing (or a short row strip), so
  Look is the mouse and the camera recentre again.
* "Reset pad to defaults" button on the tab (resets the pad slots only).
* Unit test: every bindable pad button has a label place (no two overlapping); scenario screenshot of the tab with an
  Xbox and a PlayStation family.

## Play-test

* Options > Gamepad: the drawing shows the pad's buttons, each with its action; with the DualShock the face buttons
  are ✕ ○ □ △ in colour.
* Rebind a button there with the pad, and with the mouse; the label shows the new action, the game uses it.
* Options > Controls: no pad column, less text; Options > Look: mouse rows only.
