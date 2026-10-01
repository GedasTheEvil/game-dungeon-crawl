# Stage 5: break up Dungeon

Status: implemented 2026-09-30 (the parts that pay off now, see below), verified in play 2026-09-30. Stage 5 of the
[code structure review](code-structure-review.md).

## Why

`Dungeon` has ten jobs over ~1800 lines. Some of the pain is copies (the view window worked out in seven places, the
weapon reach converted in three), some is misplaced code (the boss director inside the monster file, the debug
hitboxes reading the inventory from `world/`), some is numbers that mean something without a name (the player's
two movement widths, the trap sizes read from the render assets).

## Done

* `ViewWindow` (`dungeon.h`): the 10 x 6 cells drawn round the player and the frame origin, one definition instead
  of seven (`Draw`, `spawnInView`, `inView` / `DrawMonsters`, `drawHitboxes`, `drawArrows`, `drawFires`,
  `addLights`, `drawMechanismEffects`).
* `Item::Reach()`: tiles from `range` (tenths), for the melee check, the bow's aim and the debug view.
  `Monster::Nearby` and `Dungeon::AttackNearest` take tiles.
* The debug hitbox view gets the weapon's reach as a `HitboxView` from `graphics/draw.cpp`; `world/` no longer
  includes `ui/inventory.h` for it, nor reads `Game().render.Hitboxes`. The drawing moved to `dungeon_render.cpp`.
* The boss fight (`Boss`, `BossHealth`, `SlayBoss`, `LivingMinions`, `startBossFight`, `updateBoss`,
  `summonMinion`) is `dungeon_boss.cpp`, without GL.
* Named: `PLAYER_SCALE`, `PLAYER_BODY_HALF_WIDTH` (0.25, walls and gates) and `PLAYER_CLIMB_HEADROOM` (0.375),
  with why the body is wider than the 0.06 hitbox; `SPIKES_SCALE` / `DEATH_TRAP_SCALE` for both the drawing and the
  trap hitboxes (the simulation no longer reads the render assets). levelcheck's gate stop has a `static_assert`
  against the body width.

All 290 scenario screenshots are pixel-identical to before; levelcheck output and the path replays unchanged.

## Not done, and why

* **Separate classes for the grid, mechanisms, monsters, projectiles, decor and renderer.** The jobs are already
  one file each (`dungeon_base`, `_io`, `_mechanisms`, `_monsters`, `_boss`, `_arrows`, `_decor`, `_render`). As
  classes they would all need the map, the player position and `Game()`; that churn pays off once `Game()` is gone
  (stage 6), not before.
* **The player owns its position and physics** (`mapX` / `mapY` and the jump in `Dungeon`, the jump state in
  `Player`). It touches ~150 lines of movement code, and stage 9 (fixed timestep, held-key movement) rewrites that
  code anyway: do it there, once.
* **One player width** for walls and hits: the movement width (0.25) is four times the model's hitbox (0.06), on
  purpose. Unifying them changes how close the player walks to walls; they are named instead.
