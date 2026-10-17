# Interact prompt and the inscription glint

Status: solved 2026-10-16, play-tested (`bin/game --play tests/play/interact-prompt.txt`). Implemented 2026-10-15.
From the user, after [hieroglyph-cartouches](hieroglyph-cartouches.md). Opus: the HUD, every usable tile, and a
wall effect.

## Done

* `Interaction` (`src/world/interaction.h`) and `Dungeon::InteractVerb` (`src/world/dungeon_interact.cpp`):
  `Interact` switches on it. Besides the verbs below: Copy (an inscription without the hieroglyph skill), Light arrows
  (a brazier with a bow, plan fire-damage). A too worn inscription is Read either way.
* The mimic wakes on the key (Open) or a hit; `MIMIC_WAKE_RANGE` is gone.
* The prompt: `src/ui/interact_prompt.cpp`, layout and fade in `InteractPrompt` (`src/ui/ui_layout.h`). In a window too
  narrow for both (portrait), it moves above the HUD and its tiles.
* The glint: `glintQuad` in `src/world/dungeon_render_decor.cpp`, two additive passes of the inscription's own atlas
  cell, so the lit stele catches it; strength from `Dungeon::InscriptionGlint` (1 unread, 0.5 a copy, 0 read or worn).
* Tests: `tests/unit/interact_test.cpp` (`tests/levels/interact`), `tests/unit/ui_layout_test.cpp`,
  `tests/scenarios/solved/interact_prompt.txt`, the mimic tests updated. The spent riddle gate has no unit test of its own
  (SimWorld cannot answer a riddle); it falls under the gate's None like the entrance, which is tested.

## Why

The game shows no "press E" anywhere. A chest reads as something to use (its model, the item spinning on top), but
nothing says the interact key opens it rather than a walk over it. Worse, readable inscriptions look like the
scattered glyph decals (columns, rows, cartouches in `DECAL_DEFS`, `src/world/decor.h`, from tier 2 on): the player
cannot tell which wall can be read.

## Part 1: the prompt

While the player stands in a cell where the interact key does something, a small label shows the key and the verb:
"[E] Open", "[E] Pull", "[E] Read", "[E] Enter", "[E] Answer", "[E] Take".

* Which cells, from `Dungeon::Interact` (`src/world/dungeon_interact.cpp`) and its helpers, in the same order:
  * `Treasure`: Open. With [treasure-containers](../treasure-containers.md) (closed until opened): Open on a
    closed one, Take once open and its item shows; whichever plan lands second adds the Take.
  * `Lever` not pulled yet: Pull (a pulled lever: none).
  * `Inscription`: Read (Copy without the hieroglyph skill, once
    [skill-hieroglyphs](skill-hieroglyphs.md) is in).
  * `Door`: riddle gate (`GateRiddle`) Answer; exit (`GateExit`) Enter; teleporter with a partner Enter, not while
    jumping or falling (as `useGate`). Entrance and spent riddle gate (`GateEmpty`): none.
  * `Ankh`: Take.
  * Keys are picked up on touch: no prompt.
* One function decides it, beside `Interact`, so the two cannot drift: e.g. `Dungeon::InteractVerb()` returning an
  enum (`None`, `Open`, ...), and `Interact` switching on the same answer. A unit test (SimWorld) per verb, and one
  that `Interact` does something exactly when the verb is not `None`.
* The key: the one bound to interact, `keyCap` of its first binding (`src/input/bindings.h`), so a remapped key
  shows right ("E", "F12", "LMB").
* Look: the shared UI look ([docs/ui.md](../../ui.md)): a small dark panel with a key cap and the verb in the bottom
  right corner (the user, 2026-10-12: free today; the HUD panel holds the bottom left, the level gem the top right).
  It mirrors the HUD panel: same frame, its bottom edge on the HUD panel's bottom edge, the same margin from the
  screen edge. Its text is smaller than the status message's at the top centre (`StatusBox`, `src/ui/status_box.*`),
  about the HUD's label size. Fades in after a short delay (~150 ms) so walking past does not flicker it; hidden while a screen is open, while
  travelling through a gate (`Dungeon::Travelling`), dead, or the level won.
* No setting to switch it off (agreed with the user: always on).
* The mimic (Chest of Set) is a monster spawn, not a `Treasure`: it gets no prompt, and a chest without "[E] Open"
  would give it away. Show "[E] Open" in front of a mimic still lying in ambush too, and the key springs it (decided
  with the user, 2026-10-12: with [trapped-chests](../trapped-chests.draft.opus.md) opening any chest is a gamble), so
  the prompt never tells them apart.
  * Today the mimic wakes at `MIMIC_WAKE_RANGE` (1.5 tiles, `src/core/gameplay_config.h`), before the player can stand
    in front of it. Change the wake: an idle mimic wakes on the interact key or a hit, not on coming near; drop
    `MIMIC_WAKE_RANGE`. The [cursed mimic](../cursed-mimic.draft.opus.md) wakes the same way. The checker's threat for
    the mimic stays (the player opens chests anyway).

## Part 2: the glint on unread inscriptions

An inscription not read yet gets a faint gold glint: a soft bright band that sweeps across the quad now and then (a
few seconds apart, each inscription at its own phase, seeded by its cell), like torchlight catching fresh paint. Once
read it stops: the journal has the entry (`Journal::Inscriptions`).

* Drawn in `Dungeon::drawInscriptionTile` (`src/world/dungeon_render_decor.cpp`): a second additive pass over the
  quad with a moving gradient, or a small glint texture; no shader needed.
* With [skill-hieroglyphs](skill-hieroglyphs.md): a copy not translated yet keeps a dimmer glint until
  the skill is learnt (whichever plan lands second wires it).
* Chests get no glint: they stand out already, and a glow would fight the torch lighting.

## Tests

* Unit: the verb per tile (each case above, the pulled lever, the spent riddle gate, the teleporter mid-jump, the
  mimic); interact acts exactly when there is a verb; a mimic in ambush wakes on the key, not on coming near.
* Scenario with screenshots: the prompt at a chest, a lever and an inscription; an inscription's glint before and after
  reading it.
