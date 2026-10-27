# Loading screen: two lines

Status: draft 2026-10-27, from the user. Ready (needs nothing first).

## Problem

The loading screen (`GameState::DrawLoad`, `src/state/game_state.cpp`) prints one line under the bar:
`fonts.loading.print(10, 15, text)`. The font is monospaced (`Assets::LoadLoadingScreen`: papyrus, size 7, spacing -1,
not proportional), so every glyph advances 2.5 of the 140-wide ortho screen. From x = 10, only 52 characters fit.
"Loading Item Models [Containers, potions and amulets]" has 53, and a long monster or item name in
"Loading Monster Models [%s]" / "Loading Item Models [%s]" (`src/state/assets.cpp`) runs off the right edge too.

## Change

Two lines under the bar: what loads, and below it, the detail.

* Line 1: the step, e.g. "Loading monster models", "Loading item models", "Loading decorations".
* Line 2: the detail, e.g. the monster or item name, "Containers, potions and amulets". Empty for steps without one.

Change the progress callback from `(float percent, const char* text)` to
`(float percent, const char* step, const char* detail)` (the `Progress` alias in `assets.cpp`, `Assets::Load` in
`assets.h`, `DrawLoad`). Give each call its own two strings; don't split one string on `[`. Also update the log line in
`DrawLoad`.

Layout: the bar spans y 28 to 38. Put line 1 at y 15 (where the text is now) and line 2 below it, around y 7,
in a smaller or dimmer style (a second font size, or the same font in a darker colour). Pick by eye from a
screenshot. Both lines start at x 10, like the bar.

## Fit check

Unit test: for every monster kind and every weapon kind, the detail line's width (`chars * advance`, or
`Font::TextWidth` if testable without GL) fits in 120 (x 10 to 130). Keep the font width maths in one place, so the
test and `DrawLoad` share it.

## Test

Scenario or manual run with a screenshot of the loading screen during the item step
(`xvfb-run -a timeout -k 5 N bin/game --no-sound ...`, see AGENTS.md): both lines visible, inside the screen.
