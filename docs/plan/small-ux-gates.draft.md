# Small UX: exit gate plasma, teleporter sound, gate travel effect

Status: draft 2026-10-10, ready (needs nothing first). From the user.

## 1. Red plasma in the exit gate

Today the entrance, exit and teleporter gates share one blue plasma (`textures.portal`, `textures/effects/plasma.png`,
drawn by `Dungeon::drawPortal`, `src/world/dungeon_render.cpp`). The exit gets red plasma, so the player tells it
apart at a glance.

* Tint in the draw (a colour on `drawPortal`), or a second texture (`plasma_red.png`): implementer's choice. A tint
  keeps one texture; check it still looks like plasma, not a dark blue-red mix (a hue shift may be needed).
* Entrance and teleporters stay blue. The map view (`src/ui/map_view.cpp`) shows the exit in red too, if it draws
  gate colours.
* Check: toon mode and normal mode screenshots of an exit gate.

## 2. Sound on teleporter use

The user hears no sound when using a teleporter gate. A sound exists and is wired: `sounds/mechanisms/teleport.wav`
(1.1 s, loudness like `gate_open.wav`), played by `sim.events->Play(WorldSound::Teleport)` in `Dungeon::Teleport`
(`src/world/dungeon_io.cpp`). So first find why it is not heard:

* Is the event lost? (The jump to the partner cell the same tick, `exploreAroundPlayer`, a view reset that drops
  queued events; a per-sound cooldown.) Check with a scenario and the game log.
* Is it heard but too weak or too short to notice? Then make a clearer one: a rising whoosh with a shimmer, about
  1 s, on departure; optionally a short arrival hum at the partner gate.

Done when a teleport is clearly audible at the default volumes.

## 3. A short effect on gate travel

Today going through a gate switches the view at once: the exit gate loads the next level (`LoadCampaignLevel`), a
teleporter (also the ones into a boss room) moves the player to its partner (`Dungeon::Teleport`), both in
`src/world/dungeon_io.cpp`. Add a short effect so the jump reads as travel:

* Out: the view fades into the plasma colour (a bright swirl or a white-blue flash, ~0.3 s); in: fades back at the
  arrival (~0.3 s). Red for the exit gate (as its plasma, section 1), blue for teleporters.
* The game does not run during it (no monster hits mid-fade). A level load happens under the faded-out frame, so the
  loading screen or bar follows the fade (check `loading-bar-jumps`).
* Off with the motion effects option? Default: the fade stays (it is short), only the swirl goes. Implementer's choice.

## Tests

* Scenario with screenshots of the exit gate (red) and an entrance (blue).
* Scenario screenshots mid-fade, for a teleporter and for the exit gate.
* Unit / sim: a teleport emits `WorldSound::Teleport` and the event reaches the sound queue.
