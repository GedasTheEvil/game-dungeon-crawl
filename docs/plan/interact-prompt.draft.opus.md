# Interact prompt and the inscription glint

Status: draft 2026-10-12, ready (needs nothing first). From the user, after
[hieroglyph-cartouches](solved/hieroglyph-cartouches.md). Opus: the HUD, every usable tile, and a wall effect.

## Why

The game shows no "press E" anywhere. A chest reads as something to use (its model, the item spinning on top), but
nothing says the interact key opens it rather than a walk over it. Worse, readable inscriptions look like the
scattered glyph decals (columns, rows, cartouches in `DECAL_DEFS`, `src/world/decor.h`, from tier 2 on): the player
cannot tell which wall can be read.

## Part 1: the prompt

While the player stands in a cell where the interact key does something, a small label shows the key and the verb:
"[E] Open", "[E] Pull", "[E] Read", "[E] Enter", "[E] Answer", "[E] Take".

* Which cells, from `Dungeon::Interact` (`src/world/dungeon_interact.cpp`) and its helpers, in the same order:
  * `Treasure`: Open.
  * `Lever` not pulled yet: Pull (a pulled lever: none).
  * `Inscription`: Read (Copy without the hieroglyph skill, once
    [skill-hieroglyphs](skill-hieroglyphs.draft.sonet.md) is in).
  * `Door`: riddle gate (`GateRiddle`) Answer; exit (`GateExit`) Enter; teleporter with a partner Enter, not while
    jumping or falling (as `useGate`). Entrance and spent riddle gate (`GateEmpty`): none.
  * `Ankh`: Take.
  * Keys are picked up on touch: no prompt.
* One function decides it, beside `Interact`, so the two cannot drift: e.g. `Dungeon::InteractVerb()` returning an
  enum (`None`, `Open`, ...), and `Interact` switching on the same answer. A unit test (SimWorld) per verb, and one
  that `Interact` does something exactly when the verb is not `None`.
* The key: the one bound to interact, `keyCap` of its first binding (`src/input/bindings.h`), so a remapped key
  shows right ("E", "F12", "LMB").
* Look: the shared UI look ([docs/ui.md](../ui.md)): a small dark panel with a key cap and the verb, at the bottom
  centre above the HUD, or over the player's head (implementer's choice; the bottom is steadier while walking). Fades
  in after a short delay (~150 ms) so walking past does not flicker it; hidden while a screen is open, while
  travelling through a gate (`Dungeon::Travelling`), dead, or the level won.
* No setting to switch it off (agreed with the user: always on).
* The mimic (Chest of Set) is a monster spawn, not a `Treasure`: it gets no prompt, and a chest without "[E] Open"
  would give it away. Show "[E] Open" in front of a mimic still lying in ambush too (opening it springs the ambush as
  walking up does today, or the prompt only; implementer's choice), so the prompt never tells the two apart.

## Part 2: the glint on unread inscriptions

An inscription not read yet gets a faint gold glint: a soft bright band that sweeps across the quad now and then (a
few seconds apart, each inscription at its own phase, seeded by its cell), like torchlight catching fresh paint. Once
read it stops: the journal has the entry (`Journal::Inscriptions`).

* Drawn in `Dungeon::drawInscriptionTile` (`src/world/dungeon_render_decor.cpp`): a second additive pass over the
  quad with a moving gradient, or a small glint texture; no shader needed.
* With [skill-hieroglyphs](skill-hieroglyphs.draft.sonet.md): a copy not translated yet keeps a dimmer glint until
  the skill is learnt (whichever plan lands second wires it).
* Chests get no glint: they stand out already, and a glow would fight the torch lighting.

## Tests

* Unit: the verb per tile (each case above, the pulled lever, the spent riddle gate, the teleporter mid-jump, the
  mimic); interact acts exactly when there is a verb.
* Scenario with screenshots: the prompt at a chest, a lever and an inscription; an inscription's glint before and after
  reading it.

## Open points

* Where the prompt sits: bottom centre or over the player.
* The mimic: a prompt that springs it, or only the prompt.
