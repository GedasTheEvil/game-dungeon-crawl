# One interface for the tab screens

Status: solved 2026-11-16. Was: draft 2026-11-16, size S, needs nothing first.

## Problem

The architecture review (2026-11-16, smell 7): the inventory, map, journal and skills screens have the same five
handlers, but `draw.cpp` and every input callback in `input.cpp` listed them again in if-chains (about 50 `Screen::`
checks). A new screen meant editing every chain, and the chains had already drifted (the map got the held-button
motion but not the passive one). `ui/screen_state.h` wrapped `ui.screen == X` in one-line helpers.

## Plan

* `ui/ui_screen.h`: `UiScreen` with `Draw`, `KeyPressed`, `SpecialKeyPressed`, `MouseFunction`, `MouseMotion`; the
  four tab screens implement it.
* `UIContext::TabScreen()`: the open one, else nullptr. The frame and the input callbacks call it once instead of the
  chains; the map now gets passive motion too (it pans only while dragging, so nothing changes).
* A key goes to the open screen only: a screen a key just switched to no longer gets the same key.
* The menu, riddle and death screens keep their own rules in `input.cpp` (key capture, typing, quick load).
* `ScreenState` keeps only `IsGameplayInteractionAllowed`.
* Checked: 25 archived UI scenarios (`tests/scenarios/solved/`: inventory, journal, map, menu, riddle, screen tabs,
  skills, keys) give the same results as before.
