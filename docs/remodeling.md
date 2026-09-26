# Model remodelling

Original Blender sources are lost; models are rebuilt procedurally in Python (the script is the source).

## Files
* `tools/blender/md3.py` - MD3 reader/writer (plain Python, no Blender needed); documents the game's MD3 conventions.
* `tools/blender/md3_import.py` - load a `.md3` into Blender (welded mesh, one shape key per frame, texture from `Textures/<category>/<name>.png`).
* `tools/blender/md3_export.py` - `export_md3(obj, path, start, end)`; bakes armature/shape keys per frame.
* `tools/blender/mdl2md3.py` - one-off converter used to move from the old text `.mdl` files (still in git history).
* `tools/blender/render_sheet.py` - headless review renders + per-frame lowest-z (floor penetration) check.
  `blender -b --python tools/blender/render_sheet.py -- <model.py> /tmp/frames "worm_walk@90:0,8" "worm_attack@40#3~0,-0.6,1:9"`
  (spec `action[@yaw][^elevation][#ortho][~x,y,z target]:frames`; camera defaults from the model's `REVIEW_VIEW`), tile with `montage`.
* `tools/blender/models/common.py` - shared helpers: loft/tube/ellipsoid, chain_weights, Builder, make_material,
  finish_mesh, uv_unwrap, bake_texture, export_files. Model scripts import it (with `importlib.reload` for live iteration).
* `tools/blender/models/anubis.py` - humanoid example: primitive parts, Euler key poses, rigid props, dropped prop bone.
* `tools/blender/models/worm.py` - creature example: surface of revolution body, spine posed from a parametric curve
  (every frame keyed), hinged jaws, floor lift from jaw tips.
* `tools/blender/models/scarab.py` - six-legged example: rigid parts per bone, analytic two-bone leg IK
  (tripod gait with planted feet, body-space targets when airborne), per-frame floor fix while rolling over in the die clip.
* `tools/blender/models/rat.py` - quadruped example: one loft from rump to nose blended over hips/chest/head bones, trot gait
  with two-bone leg IK (two strides per walk clip), FK tail chain laid onto the floor where it would sink (limp in the die clip),
  per-frame floor fix from a numpy copy of the skinning. Bakes two textures on the same UVs: `rat.png` and `rat_giant.png`
  (near-black mangy fur, red eyes); `RAT_TEX=giant` shows the giant one in review renders.
* `tools/blender/models/bat.py` - flying monster example: wing arm + four finger bones posed by FK deformation matrices, double-sided
  membrane grids between fingers / arm / leg with blended weights (they stretch and crumple when folding). Clips: fly (move, the
  normalization reference; body height fixed, the engine flies it), attack, die (floor-fixed every frame to fly frame 0's lowest point),
  idle (`bat_idle.md3`, hanging head down by the feet, feet at a constant height 0.199 wingspans above the origin). The wing tips dip
  0.25 wingspans below the origin on the downstroke (`BAT_WING_DIP`). Bakes `bat.png` and `bat_giant.png` on the same UVs;
  `BAT_TEX=giant` shows the giant one in review renders. Sounds: `tools/audio/bat_sounds.py` (`Sounds/bat_{att,die}.wav`).
  Engine: `monster::Fly` (`flies = true`): hangs from `BAT_CEILING` by the idle clip's top, swoops through the player and back
  (`BAT_*` in `src/input/gameplay_config.h`), falls to the floor on death.
* `tools/blender/models/archeologist.py` - player example: anubis-style humanoid built facing +Y and turned 180 by the rig object,
  per-frame root height from the lowest point (feet, knees, body) instead of hand-keyed root z, hat dropped on death.
* `tools/blender/models/plant.py` - static monster example: lathed jar, FK bone chains (stalk, vines) with per-bone Euler
  angles from pose parameters, hinged petals, poses eased off by bisection so nothing sinks through the floor.
* `tools/blender/models/decor.py` - ten static corridor props (web, pottery, canopic jars, rubble, sand drift, skeleton,
  brazier, offerings, scrolls, ushabti) plus the wall torch (`decor_torch`, placed by `Dungeon::scatterTorches`, up to one per
  5 cells of a row) in tile units, lighting baked into the texture (sun from the camera side + AO),
  drawn textured only (no toon pass, no Centrify). `-- --export` writes `Models/decorations/decor_<name>.md3` + `Textures/decorations/decor_<name>.png`;
  `--review out.png` renders a line-up, `--only web,sand` limits the build; extents are printed (engine jitter table in
  `src/world/dungeon_decor.cpp`). Placement: `Dungeon::scatterDecorations` (30% of walkable empty floor cells, seeded by the level file name).
* `tools/blender/models/ladder.py` - ladder pieces for `Ladder` cells, same tile units and baked lighting as `decor.py`: two styles
  (`wood`: acacia poles with rope-lashed rungs; `vine`: two twisted lianas with thin vines as holds), each with three
  interchangeable middle pieces `a`/`b`/`c` plus `top` (wood: roped to a beam between the side walls; vine: roots creeping along
  under the ceiling) and `bottom` (on the floor). Rails sit at x = +-0.1, y = -0.04 and match at z = 0 / 1, so pieces stack in any
  order; holds every 1/12 tile (for the climbing animations to come). `-- --export` writes `Models/ladders/ladder_<style>_<piece>.md3` +
  `Textures/ladders/ladder_<style>_<piece>.png`; `--review out.png [--view z,scale]` renders both styles as stacked shafts.
  Placement: `Dungeon::scatterLadders` (one style per shaft, no middle piece twice in a row, wooden pieces mirrored at random;
  lianas are never mirrored, their twist would kink at the seams). Tables: `LADDER_*` in `src/world/decor.h`.
* `tools/blender/models/mechanism.py` - level mechanics, same tile units and baked lighting as `decor.py`: `key` (upright ankh key,
  oval gem in the loop), `gate` (bronze portcullis across the corridor: plane y-z at x = 0, 0.21 thick, full tile depth and height,
  stone lintel and back stile, lock cartouche on the front face, gem medallion on both long sides, spiked feet; slides up into the
  ceiling as a whole), `lever_base` (wall plate at z 0.23..0.57 with slot, bearing boss and gem) + `lever_handle` (origin = pivot, points
  +Z; pivot at `LEVER_PIVOT` = (0, -0.05, 0.42) in the base frame, swung +-35 deg around the depth axis), `rock` (faceted sandstone
  boulder ~0.38 across) and `ceiling_crack` (loose stones sagging out of the ceiling at z = 1, dark gaps, sand trickle). Gate, lever
  and crack use the decor frame (origin on the back wall, `drawDecorTile`'s transform); key and rock are centred on their own
  vertical axis with the lowest point at z = 0 (draw from the tile centre, like the old ankh item; the key spins around its shaft).
  Key, gate and lever_base have one texture per lock colour on the same UVs (`red` carnelian, `blue` lapis, `green` turquoise,
  `gold` yellow amber; only gems and painted accents differ). `-- --export` writes `Models/mechanisms/<model>.md3` +
  `Textures/mechanisms/<model>[_<colour>].png`; `--review out.png` (textured with `--bake` or `--export`) renders colour line-ups
  (`out.png`, `out_small.png`) and two corridor shots from the game camera (`out_corridor{1,2}.png`); `--only key,gate`.
* `tools/textures/decals.py` - wall decal atlas `Textures/decorations/decals.png` (RGBA, 4x4 cells of 256 px, loaded with mipmaps): cracks,
  vines, roots, seepage, hieroglyph panels, cartouche, eye of Horus, winged sun, papyrus, dry grass, creeper, moss. Cell order =
  `DECAL_DEFS` in `src/world/decor.h` (anchor: free / ceiling / floor, quad size). `python3 tools/textures/decals.py`.
  Placement: `Dungeon::scatterDecals` (35% of cells with a visible back wall, one decal per cell).
* Rebuild all game files of a model: `blender -b --python tools/blender/models/<name>.py -- --export`
  (writes `Models/<category>/<name>{,_att,_die}.md3`, `Textures/<category>/<name>.png`, saves `tools/blender/models/<name>.blend`).
  In live Blender (MCP): `exec(open(p).read(), g); g["build"](bake=False)` for quick iteration.
* Engine side: `src/graphics/ani.cpp`/`ani.h` (loader), `src/graphics/textures.cpp` (PNG textures), `src/graphics/shader.cpp` (toon shading, ramp in `Textures/Shader.txt`), model/texture wiring in `src/state/game_state.cpp`.
* Lighting: `src/graphics/lighting.cpp` (GLSL per-pixel point lights over a dark ambient; player, torches, braziers, oil lamps;
  toon mode stays unlit) and `src/graphics/fire.cpp` (stateless fire particles). Flame origins per prop: `BRAZIER_FIRE`,
  `LAMP_FIRE`, `TORCH_FIRE` in `src/world/dungeon_decor.cpp`; keep them in sync with the geometry in `decor.py`.
* `tools/audio/jump_sound.py` - synthesizes `Sounds/Jump.wav` (boot scuff, effort "hup", cloth whoosh; 16-bit PCM).
* `ModelViewer/viewer <file.md3> [seconds]` (`make model-viewer`) - check exported files in the real engine.

## Format and engine conventions
* Models are Quake 3 MD3 (binary, int16 positions, 16-bit normals in every frame, <= 4096 verts per surface,
  exporter splits surfaces). Game space Y-up, counter-clockwise triangles, each file scaled to fill the int16 range
  (header name holds `;unit=`, which the loader applies so all files of a model share real units). Loader: `AnimatedModel::Load` in `src/graphics/ani.cpp`.
* Blender space: Z-up. Facing depends on the monster's `rotA` in `game_state.cpp`: Anubis (180) faces +Y, worm (0) faces -Y.
  Check the old model's facing before remodelling.
* The engine normalizes a monster by the walk-slot file (`<name>.md3`) frame 0: largest dimension -> 1, centred in x/z,
  min Y on the floor (`Centrify`); the attack and die files get the same transform (`Normalize`, `monster::loadModel`),
  so their frame 0 may differ. Keeping frame 0 the same pose in all three files still gives the smoothest switches.
  Single-file models (items, props) are centred on their own frame 0.
* Animation states (`ModelState`): Idle, Move, Attack, Die, Jump, Climb, one file per clip. The file list (`ClipFiles` in
  `src/entities/monster.h`) names each file's suffix and whether it loops; its first file is the reference: required, normalizes all
  clips and stands in for a missing optional clip. `MONSTER_CLIPS`: `<name>.md3` Move, `_att` Attack, `_die` Die, optional `_idle` Idle.
  `PLAYER_CLIPS`: `<name>.md3` Idle, `_walk` Move, `_die`, optional `_jump`, `_climb`.
* The player: `archeologist.md3` = idle (standing), `archeologist_walk.md3` = walk cycle (while moving),
  `archeologist_die.md3` = death, `archeologist_jump.md3` = forward jump (optional `<name>_jump.md3`, `ModelState::Jump`; restarts on every
  jump, plays once and holds the landing crouch; the game moves the body, so the pelvis stays at standing height),
  `archeologist_climb.md3` = climbing a ladder (optional `<name>_climb.md3`, `ModelState::Climb`): back to the camera (drawn at rotA 180,
  moved `PLAYER_CLIMB_DEPTH` towards the wall so the fists meet the rungs), hands and feet on IK targets in `archeologist.py`. The engine sets
  its frame from the height (`Dungeon::ClimbPhase`, one cycle per tile; the clock does not advance it), so it runs backwards going down
  and holds when the player stops. Shown while `Dungeon::PlayerOnLadder` (a Ladder cell, off the floor, within reach of the ladder);
  sideways moves on a ladder use the walk cycle. The weapon is hidden while climbing.
  The weapon is drawn separately in front of the chest at ~3/4 height, so the fists stay raised there.
  `ModelViewer` looks for `Textures/<category>/<stem>.png` (the model's sub-directory under `Models/`).
* Monsters need three files: `<name>.md3` move (loops), `<name>_att.md3` attack (loops), `<name>_die.md3` die (plays once, holds last frame),
  plus an optional `<name>_idle.md3` (loops; the bat hanging on the ceiling). Monsters without it show the move clip when idle.
  Any frame count per file (Anubis 26, worm 32/32/40, scarab 24/26/32, plant 32/26/36, rat 24/24/30, bat 12/12/24 + idle 24, archeologist 32/20/30 + jump 10 + climb 24); engine plays ~14 fps. Loops: key frame N = frame 0, export 0..N-1.
* Textures: PNG (`bake_texture` in `common.py` saves with Blender `file_format="PNG"`, RGB), 1024x1024 for the remodelled monsters and player.
  Loaded by `Textura::LoadPNG` (`src/graphics/textures.cpp`, stb_image); an alpha channel is kept if present, rows are flipped so UV v=0 is the image bottom.
* Sizes: Anubis 8.2k tris ~1.5 MB/file, worm 6.4k tris ~1.7-2.1 MB/file, scarab 12.5k tris ~2.3-3.0 MB/file, plant 10.9k tris ~2.5-3.3 MB/file, rat 5.8k tris ~1.1-1.4 MB/file, bat 6.2k tris ~0.64-1.17 MB/file, archeologist 7.3k tris ~1.1-1.7 MB/file; all 31 models load in ~0.2 s.

## Status
Paths relative to `Models/` and `Textures/`. UI screens are in `Textures/ui/`, dungeon wall textures in `Textures/dungeon/`, `plasma.png` in `Textures/effects/`.

| Model | Files | Texture | Status |
|---|---|---|---|
| Anubis (monster) | `monsters/anubis{,_att,_die}.md3` | `monsters/anubis.png` | remodelled |
| Worm (monster) | `monsters/worm{,_att,_die}.md3` | `monsters/worm.png` | remodelled (man-eating worm) |
| Scarab (monster) | `monsters/scarab{,_att,_die}.md3` | `monsters/scarab.png` | remodelled (giant golden Scarabaeus sacer) |
| Rat, giant rat (monsters) | `monsters/rat{,_att,_die}.md3` | `monsters/rat.png`, `monsters/rat_giant.png` | new (tomb rat; the giant rat uses the same files with its own texture) |
| Bat, giant bat (monsters) | `monsters/bat{,_att,_die,_idle}.md3` | `monsters/bat.png`, `monsters/bat_giant.png` | new (tomb bat; the giant bat uses the same files with its own texture) |
| Plant (monster) | `monsters/plant{,_att,_die}.md3` | `monsters/plant.png` | remodelled (tomb lotus in a painted jar; walk file = idle) |
| Player | `characters/archeologist{,_att,_die,_jump,_climb}.md3` | `characters/archeologist.png` | remodelled (archaeologist with fedora) |
| Sphinx, ankh, questionmark | `props/{sphinx,ankh,questionmark}.md3` | `props/sphinx.png`, `props/ankh.png`, `items/gold.png` | old (static) |
| Columns (old ladder, with a plasma quad) | `props/columns.md3` | `props/columns.png` | unused since the ladders |
| Ladders (2 styles x 5 pieces) | `ladders/ladder_<style>_<piece>.md3` | `ladders/ladder_<style>_<piece>.png` | new (static, `ladder.py`) |
| Items: club, sword, spear, bow, potion, chest | `items/club.md3`, ..., `items/tchest.md3` | `items/club.png`, ..., `items/tchest.png` (bow uses `items/gold.png`, the old scarab texture) | old (static) |
| Spikes trap | `traps/spikes.md3` | `traps/spikes.png` | old (static) |
| Corridor decorations (10 props) | `decorations/decor_<name>.md3` | `decorations/decor_<name>.png` | new (static, `decor.py`) |
| Wall torch | `decorations/decor_torch.md3` | `decorations/decor_torch.png` | new (static, `decor.py`); flame = `Fire::TORCH` particles |
| Keys (4 lock colours) | `mechanisms/key.md3` | `mechanisms/key_<colour>.png` | new (static, `mechanism.py`) |
| Key gate (4 lock colours) | `mechanisms/gate.md3` | `mechanisms/gate_<colour>.png` | new (static, `mechanism.py`) |
| Wall lever (4 lock colours) | `mechanisms/lever_base.md3`, `mechanisms/lever_handle.md3` | `mechanisms/lever_base_<colour>.png`, `mechanisms/lever_handle.png` | new (static, `mechanism.py`) |
| Falling rock, ceiling crack | `mechanisms/rock.md3`, `mechanisms/ceiling_crack.md3` | `mechanisms/rock.png`, `mechanisms/ceiling_crack.png` | new (static, `mechanism.py`) |
| Wall decals (16) | - | `decorations/decals.png` | new (generated, `tools/textures/decals.py`) |
