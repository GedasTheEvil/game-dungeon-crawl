# Modelling

How a new model gets made. Every model is a Python script in `tools/blender/models/` that builds the mesh, rig,
texture and clips from code: the script is the source. Run it with Blender 5 (the path in AGENTS.md; `blender` below
means that one). How the original models were rebuilt, and notes per model: [remodeling.md](remodeling.md).

## Steps
1. Reference images in `docs/plan/references/<model>/` (not in git).
2. Copy the closest script (table below): its docstring header (MCP / CLI usage, Blender space, facing, clips,
   textures), `build()`, `export()` and CLI flags. Shared helpers come from `common.py`.
3. Iterate in live Blender (MCP) or headless with `render_sheet.py`; check the motion (`--onion`, `--paths`, `MINZ`,
   `SEAM`, `SLIDE`).
4. `-- --export`, then `md3_stats.py` and `bin/model-viewer` on the files.
5. Wire it in: a row in `KINDS` (`src/world/monster_kinds.cpp`) or `ITEMS` (`src/world/items.cpp`), sounds from a
   `tools/audio/<name>_sounds.py` (`media/sounds/<category>/<name>_{wake,att,die}.wav`, optional `_jump`, `_att_ranged`), a scenario to see it in the
   dungeon ([testing.md](testing.md)).
6. A monster with a boss: the boss texture on the same UVs (`--boss-texture`, `<NAME>_TEX=<boss>` in review renders).
7. Notes on the model (clips, frame counts, numbers the engine reads from it) go in the script's docstring, not here;
   add a row to the table below.

## Tools
* `tools/blender/md3.py` - MD3 reader/writer (plain Python, no Blender needed); documents the game's MD3 conventions.
* `tools/blender/md3_import.py` - load a `.md3` into Blender (welded mesh, one shape key per frame, texture from `media/textures/<category>/<name>.png`).
* `tools/blender/md3_export.py` - `export_md3(obj, path, frame_start=None, frame_end=None)`; bakes armature/shape keys per frame.
* `tools/blender/render_sheet.py` - headless review renders and motion checks.
  `blender -b --python tools/blender/render_sheet.py -- <model.py> /tmp/frames "worm_walk@90:0,8" "worm_attack@40#3~0,-0.6,1:9"`
  (spec `action[@yaw][^elevation][#ortho][~x,y,z target]:frames`, frames `all` for the whole clip; camera defaults from the
  model's `REVIEW_VIEW`), tile with `montage`. Motion as images (stills show poses, not timing):
  `--onion` adds `<spec>_onion.png` (the spec's frames over each other, older ones bluer and fainter), `--paths[=bone,...]`
  adds `<spec>_paths.png` (the bone tips' paths over the whole clip, a dot per frame so the spacing shows the speed, the
  first frame dimmed under them; default the deforming leaf bones: feet, hands, tail tip, jaw). A top view (`^89`) shows
  foot placement and sway. Printed per clip: `MINZ` (lowest point per frame: floor penetration), `SEAM` (last frame ->
  frame 0 jump against the median frame step; about 1 for a loop, a die clip is expected high), `SLIDE` (median horizontal
  step of the vertices on the floor per frame; an in-place walk wants an even step, `-` = nothing on the floor), with
  `--intersect` also `INTERSECT` (self-intersecting face pairs per frame; rigid parts overlap by design, so watch the
  jumps, not the baseline).
* `tools/blender/md3_stats.py` - frames, triangles, bytes and extents (frame 0, all frames) of exported files; `--diff [REV]`
  compares with the files at a git revision (default `HEAD`): extents change and the largest corner move.
  `python3 tools/blender/md3_stats.py --diff media/models/monsters/rat*.md3` after an `--export`.
* `docs/plan/references/<model>/` - reference images (photos, concept art) to compare renders against for shape and
  proportions; kept out of git (its `.gitignore`), like `docs/plan/screenshots/`.
* `tools/blender/models/common.py` - shared helpers: loft/tube/ellipsoid, chain_weights, Builder, make_material,
  finish_mesh, uv_unwrap, bake_texture, export_files. Model scripts import it (with `importlib.reload` for live iteration).
* Rebuild all game files of a model: `blender -b --python tools/blender/models/<name>.py -- --export`
  (writes `media/models/<category>/<name>{,_att,_die,...}.md3`, `media/textures/<category>/<name>.png`, saves `tools/blender/models/<name>.blend`;
  the rig-less `lotus.py` and `ushabti.py` save none).
  In live Blender (MCP): `exec(open(p).read(), g); g["build"](bake=False)` for quick iteration.
* `bin/model-viewer <file.md3> [seconds] [options]` (`make model-viewer`, or `make run-model-viewer ARGS="..."`) - check exported files in the real engine.
  Space cycles the model's clips, T its textures, L the lighting (flat, the game's with the player's light, toon with
  ink lines), + / - change the loop speed, left-drag turns the model. Screenshots in the game's renderer (texture
  filtering, int16 steps, `Centrify`, lighting), headless: `xvfb-run -a -s "-screen 0 1280x1024x24" bin/model-viewer
  media/models/monsters/rat.md3 --shot /tmp/shots --frames 0,6,12 --yaw 90 --light toon --texture rat_giant --size 480x480`
  writes `<stem>_<frame>.png` and exits (`--frames` default all; `--pitch`). The model in the dungeon, among walls and
  torches: a scenario ([testing.md](testing.md)), e.g. `tests/scenarios/solved/props.txt`.

## Format and engine conventions
* Models are Quake 3 MD3 (binary, int16 positions, 16-bit normals in every frame, <= 4096 verts per surface,
  exporter splits surfaces). Game space Y-up, counter-clockwise triangles, each file scaled to fill the int16 range
  (header name holds `;unit=`, which the loader applies so all files of a model share real units). Loader: `Md3Mesh::Load` in `src/entities/md3_mesh.cpp`.
* Blender space: Z-up. Facing depends on the monster's `rotA` in `KINDS` (`src/world/monster_kinds.cpp`): Anubis (180) faces +Y, worm (0) faces -Y.
  Check the facing in `KINDS` first.
* The engine normalizes a monster by its reference clip's frame 0 (the walk-slot file `<name>.md3`; `_idle` for the mimics, `AMBUSH_CLIPS`, `AMBUSH_HOP_CLIPS`): largest dimension -> 1, centred in x/z,
  min Y on the floor (`Centrify`); the attack and die files get the same transform (`Normalize`, `LoadClips` in `src/entities/model_info.cpp`),
  so their frame 0 may differ. Keeping frame 0 the same pose in all three files still gives the smoothest switches.
  Single-file models (items, props) are centred on their own frame 0.
* Animation states (`ModelState`): Idle, Move, Attack, Die, Jump, Climb, Rise, AttackRanged, one file per clip. The file list (`ClipFiles` in
  `src/entities/model_info.h`) names each file's suffix and whether it loops; its first file is the reference: required, normalizes all
  clips and stands in for a missing optional clip. `ClipFilesOf` picks a monster's list by its `locomotion` and `ranged` in `KINDS`.
  `MONSTER_CLIPS`: `<name>.md3` Move, `_att` Attack, `_die` Die, optional `_idle` Idle, optional `_jump` Jump (plays once), optional `_climb` Climb.
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
  `model-viewer` looks in `media/textures/<category>/` (the model's sub-directory under `media/models/`) for `<stem>.png`, then the
  base model's `<base>.png` (`anubis_att` -> `anubis`; the base is the shortest `_` cut with its own `.md3`), then the
  variants `<base>_*.png` (`anubis_boss`), which T cycles through. A potion vessel (`items/potion_flask`) has no texture
  of its own: it gets its potions' textures from the game's table (`PotionDef::texture`), also cycled by T.
* Monsters need three files: `<name>.md3` move (loops), `<name>_att.md3` attack (loops), `<name>_die.md3` die (plays once, holds last frame),
  plus an optional `<name>_idle.md3` (loops; the bat hanging on the ceiling). Monsters without it show the move clip when idle.
  The mimic (`AMBUSH_CLIPS`) requires `_idle` (the closed chest) and uses it as the reference clip; the cursed mimic
  (`AMBUSH_HOP_CLIPS`, files `mimic_cursed*`) the same, its move clip a hop, plus an optional `_jump`.
  Optional `<name>_jump.md3` (plays once, holds the last frame; the giant rat's, giant scarab's and cursed mimic's leap) and `media/sounds/<category>/<name>_jump.wav`.
  Optional `<name>_climb.md3` (loops, back to the camera; rat, scorpion, mummy, mummy priest, Anubis).
  The mummy (`ENTOMBED_CLIPS`) adds a required `_idle` (dormant in its coffin) and `_rise` (`ModelState::Rise`, suffix `_rise`, plays once:
  its wake clip, climbing out; the last frame is walk frame 0). The cobra (`COILED_CLIPS`), the blue lotus and the ushabti
  (`FLOATING_CLIPS`) require `_idle` and `_rise` too.
  A ranged attack: walkers and the mummy priest (`RANGED_CLIPS`, `ENTOMBED_RANGED_CLIPS`) play `_att` once as the ranged clip;
  the cobra (`COILED_CLIPS`) and the baboon (`THROWER_CLIPS`, `RangedRules::ownClip`) have their own `_att_ranged` (plays once),
  and so do the rollers (`ROLLER_CLIPS`, `MonsterKind::roll`: the giant scarab's and Khepri's dung ball, `scarab.py`). Sound `media/sounds/<category>/<name>_att_ranged.wav`.
* Any frame count per file; the engine plays ~14 fps. Loops: key frame N = frame 0, export 0..N-1.
* Textures: PNG (`bake_texture` in `common.py` saves with Blender `file_format="PNG"`, RGB), 1024x1024 for monsters and the player (the mimics 1024x512, the lotus and the ushabti 512x512).
  Loaded by `Texture::LoadPNG` (`src/graphics/textures.cpp`, stb_image); an alpha channel is kept if present, rows are flipped so UV v=0 is the image bottom.
* Engine side: `src/entities/md3_mesh.cpp` (MD3 loader), `src/entities/model_info.cpp` (clip files, normalization), `src/graphics/animated_model.cpp` (drawing), `src/graphics/textures.cpp` (PNG textures), model/texture paths in `src/world/monster_kinds.cpp` (`KINDS`) and `src/world/items.cpp` (`ITEMS`; the player in `game_state.cpp`).
* Lighting: `src/graphics/lighting.cpp` (GLSL per-pixel point lights over a dark ambient; player, torches, braziers, oil lamps;
  toon mode (F1) snaps the light to cel bands), `src/graphics/ink.cpp` (toon ink outlines: depth-based post pass,
  lines on silhouettes and creases of anything that writes depth) and `src/graphics/fire.cpp` (stateless fire particles). Flame origins per prop: `BRAZIER_FIRE`,
  `LAMP_FIRE`, `TORCH_FIRE` in `src/world/decor_scatter.cpp`; keep them in sync with the geometry in `decor.py`.

## Model scripts
Start from the one closest to the new model. Details per model: its docstring and [remodeling.md](remodeling.md).

| Script | Builds | Technique to copy |
|---|---|---|
| `anubis.py` | Anubis, its boss | humanoid: primitive parts, Euler key poses, rigid props, dropped prop bone; ranged clip, climb |
| `archeologist.py` | the player | humanoid turned 180 by the rig, root height from the lowest point, climb on IK targets |
| `mummy.py` | mummy | humanoid with cloth strips (verlet), idle in a coffin and a rise clip |
| `mummy_priest.py` | mummy priest (mummy boss) | a subclass of `mummy.py`'s `Mummy`: own head, props and poses; staff bone keyed per clip; ranged clip `--measure` |
| `worm.py` | worm | surface of revolution, spine posed from a curve, hinged jaws |
| `cobra.py` | cobra | tube along a spine chain posed by heading / elevation / roll, ranged clip |
| `rat.py` | rat, giant rat | quadruped: one loft over bones, trot with two-bone IK, FK tail on the floor, jump, climb |
| `crocodile.py` | crocodile, Sobek | `rat.py` style, idle floated at the water surface |
| `hippo.py` | hippo | `crocodile.py` style: barrel body, huge hinged jaw (the yawn), ear bones, idle with only eyes, ears and nostrils on top |
| `scarab.py` | scarab, giant scarab | six legs, rigid parts per bone, analytic IK, tripod gait, jump |
| `scorpion.py` | scorpion, giant, queen | eight legs, FK tail blended between poses, climb |
| `camel_spider.py` | camel spider | `scorpion.py`'s legs and palps (IK), sawing chelicerae on hinged jaws, cone bristles, idle clip |
| `baboon.py` | baboon, Babi (`--boss`: reshaped, own files) | `rat.py` style quadruped (loft over bones, two-bone IK legs, FK tail, climb) with world-point hand targets (rearing, raised hands), a mane loft with flat locks, a prop bone scaled to zero (the coal); ranged clip `--measure` |
| `bat.py` | bat, giant, vampire | flying: FK deformation matrices, membrane grids, hanging idle |
| `mimic.py` | mimic, cursed mimic | ambush monster built on an item's mesh (`items.build_chest`); `--cursed`: own move (hop) and jump clips, the texture recoloured by colour class |
| `egg_cluster.py` | egg cluster | rooted monster, a rigid bone per part |
| `lotus.py` | blue lotus | no rig: every frame rebuilt from pose numbers, written with `md3.py` |
| `ushabti.py` | living ushabti | `lotus.py` style: rigid parts posed from numbers, a waddle on the base edges, the die clip breaks the parts apart |
| `plant.py` | plant | static monster, FK chains, poses eased off the floor by bisection |
| `items.py` | weapons, missiles, potions, chest, amulets | static items in metres; bow draw and chest opening frames from shape keys |
| `containers.py` | treasure containers: coffer, basket, canopic jars, sealed jar, offering table | items in metres, the opening clip as shape keys: rigid parts moved per frame (lids, heads, shards), a cloth surface |
| `props.py` | gateway, ankh, question mark, spikes, teleporter | static props in tile units |
| `decor.py` | corridor and water decorations, torch, coffins | static, tile units, lighting baked in |
| `ladder.py` | ladder pieces | stackable pieces in tile units |
| `mechanism.py` | key, gate, lever, rock, crack, dart trap, wall spikes, fire vent, ring platform, its housing and ring | static, one texture per lock colour |

Generated textures (no model): `tools/textures/decals.py`, `surfaces.py`, `hud_icons.py`.
