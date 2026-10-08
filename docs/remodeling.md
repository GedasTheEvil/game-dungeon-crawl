# Model remodelling

Original Blender sources are lost; models are rebuilt procedurally in Python (the script is the source).

## Files
* `tools/blender/md3.py` - MD3 reader/writer (plain Python, no Blender needed); documents the game's MD3 conventions.
* `tools/blender/md3_import.py` - load a `.md3` into Blender (welded mesh, one shape key per frame, texture from `textures/<category>/<name>.png`).
* `tools/blender/md3_export.py` - `export_md3(obj, path, frame_start=None, frame_end=None)`; bakes armature/shape keys per frame.
* `tools/blender/mdl2md3.py` - one-off converter used to move from the old text `.mdl` files (still in git history).
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
  `python3 tools/blender/md3_stats.py --diff models/monsters/rat*.md3` after an `--export`.
* `docs/plan/references/<model>/` - reference images (photos, concept art) to compare renders against for shape and
  proportions; kept out of git (its `.gitignore`), like `docs/plan/screenshots/`.
* `tools/blender/models/common.py` - shared helpers: loft/tube/ellipsoid, chain_weights, Builder, make_material,
  finish_mesh, uv_unwrap, bake_texture, export_files. Model scripts import it (with `importlib.reload` for live iteration).
* `tools/blender/models/anubis.py` - humanoid example: primitive parts, Euler key poses, rigid props, dropped prop bone.
  `--boss-texture` bakes only `anubis_boss.png` (the Anubis boss: obsidian, carnelian and gold) on the same UV layout.
* `tools/blender/models/worm.py` - creature example: surface of revolution body, spine posed from a parametric curve
  (every frame keyed), hinged jaws, floor lift from jaw tips.
* `tools/blender/models/scarab.py` - six-legged example: rigid parts per bone, analytic two-bone leg IK
  (tripod gait with planted feet, body-space targets when airborne), per-frame floor fix while rolling over in the die clip.
  The jump clip (`scarab_jump.md3`, 10 frames, plays once: crouch, spring off with the elytra lifted, legs tucked, landing
  crouch; per-frame floor fix for the hanging tarsi) is the giant scarab's leap. Bakes `scarab.png` and `scarab_giant.png`
  (obsidian and carnelian, red eyes) on the same UVs; `SCARAB_TEX=giant` shows the giant one in review renders.
* `tools/blender/models/rat.py` - quadruped example: one loft from rump to nose blended over hips/chest/head bones, trot gait
  with two-bone leg IK (two strides per walk clip), FK tail chain laid onto the floor where it would sink (limp in the die clip),
  per-frame floor fix from a numpy copy of the skinning. The jump clip (`rat_jump.md3`, 10 frames, plays once: crouch, push-off,
  stretched in the air, paws reaching down, landing crouch) stays on the floor; the engine moves it along the arc
  (`Monster::UpdateJump`, `MONSTER_JUMP_*` in `src/core/gameplay_config.h`). Bakes two textures on the same UVs: `rat.png` and `rat_giant.png`
  (near-black mangy fur, red eyes); `RAT_TEX=giant` shows the giant one in review renders.
* `tools/blender/models/bat.py` - flying monster example: wing arm + four finger bones posed by FK deformation matrices, double-sided
  membrane grids between fingers / arm / leg with blended weights (they stretch and crumple when folding). Clips: fly (move, the
  normalization reference; body height fixed, the engine flies it), attack, die (floor-fixed every frame to fly frame 0's lowest point),
  idle (`bat_idle.md3`, hanging head down by the feet, feet at a constant height 0.199 wingspans above the origin). The wing tips dip
  0.25 wingspans below the origin on the downstroke (`BAT_WING_DIP`). Bakes `bat.png` and `bat_giant.png` on the same UVs;
  `BAT_TEX=giant` shows the giant one in review renders. `--boss-texture` bakes only `bat_vampire.png` (the vampire bat
  boss: blue-black fur, blood-red veins, crimson eyes). Sounds: `tools/audio/bat_sounds.py` (`sounds/monsters/bat_{att,die}.wav`).
  Engine: `Monster::Fly` (`Locomotion::Fly` in `KINDS`, `src/world/monster_kinds.cpp`): hangs from `BAT_CEILING` by the idle clip's top, swoops through the player and back
  (`BAT_*` in `src/core/gameplay_config.h`), falls to the floor on death.
* `tools/blender/models/mimic.py` - ambush monster example: the shell is `items.build_chest` itself (same vertices, UVs read back from
  `treasure_chest.md3`, `treasure_chest.png` copied unchanged into the left half of the 1024 x 512 `mimic.png`; the mouth parts are baked on
  their own into the right half), so the rest pose is the item vertex for vertex: frame 0 of `mimic_idle.md3`, the mimic's normalization reference
  (`AMBUSH_CLIPS` in `src/entities/character_model.h`), has the chest's bounding box (to the int16 step). Everything else hides inside at rest: lower fangs and gums in the walls (slide up), upper fangs lying flat in the lid
  frame (fold down), tongue and a fleshy mouth floor under the gold heap (the heap sinks into the box). Rigid bones posed by deformation
  matrices (as in `bat.py`), feet on a `base` bone so the legs shear under the twisting box, per-frame floor fix. Clips: walk 32 (loops on the awake
  pose, the same as attack frame 0: four lid snaps, sways and twists), attack 22 (loops from the awake pose: gape, lunge
  0.17 m towards -Y, the tongue lashing 0.77 normalized units past the chest front, snap, shake), die 30 (from the awake pose: shudders, tongue limp, lid
  slams, the gold regrows under it, lid falls open; last frame = rest pose, swapped for the real chest), idle 42 (the reference: still chest,
  the lid dips 4.5 deg and the box swells 1.2% once per loop).
  Sounds: `tools/audio/mimic_sounds.py` (`sounds/monsters/mimic_{wake,att,die}.wav`).
  Mummy sounds: `tools/audio/mummy_sounds.py` (`sounds/monsters/mummy_{wake,att,die}.wav`).
* `tools/blender/models/archeologist.py` - player example: anubis-style humanoid built facing +Y and turned 180 by the rig object,
  per-frame root height from the lowest point (feet, knees, body) instead of hand-keyed root z, hat dropped on death.
* `tools/blender/models/mummy.py` - entombed monster: anubis-style humanoid in spiral bandages (pattern-uv material), loose strips as
  two-bone chains run by a verlet pass (gravity, lag, floor, coffin). Clips: walk (stiff shamble, arms reaching; frame 0 centred on
  x = y = 0, the reference), attack (two-handed overhead smash), die (folds forward, root squashed into a heap), idle (`_idle`: on its
  back in the coffin, head at +X = game left, arms crossed, centred, back 0.6 world units up), rise (`_rise`: sits up, legs over the
  front rim, onto its feet, two steps, reaches; last frame = walk frame 0). Rise is authored in the coffin's frame: the engine draws
  idle/rise 14.4 world units towards the wall and smoothsteps that to 0 over rise t 0.3..0.75, the script adds the inverse slide to the
  root. `-- --coffin-check out_dir` builds `decor.py`'s coffin, places the dormant/rising mummy at the engine's offsets (tile units)
  and counts mummy vertices inside the coffin's walls per frame and halfway between frames, plus game/side/close renders.
* `tools/blender/models/crocodile.py` - Nile crocodile, built like `rat.py` (faces +Y, rotA 180): one flat loft for the body
  blended over hips / chest / head bones plus a leaf `breath` bone (uniform scale about the belly, swells the flanks in the idle),
  flat-topped head with the eyes and the nostril bump raised on it, lower jaw on a hinge, interlocking teeth along the jaw line,
  sprawling two-bone IK legs, FK tail chain (laterally flattened, about as long as the body) laid onto the floor with a clearance that
  follows the body roll, keeled scute rows on the back and a double tail crest merging into one. Clips: walk 24 (one lateral-sequence
  stride, tail swaying; the reference, about 3.0 long x 0.38 high x 0.89 wide), attack 22 (loops from walk frame 0: gape with the head
  thrown up, lunge, snap, head shake), die 30 (convulsion, rolls onto its back, legs limp, tail still), idle 32 (`crocodile_idle.md3`:
  flat on the belly, legs tucked along the body, head low, breath swell and slight tail sway; 0.26 high, the back scutes are the top,
  eyes at 0.95 and nostrils at 0.96 of the idle height, the smooth hide at most 0.91, so the engine floats it at the water surface with only
  eyes, nostrils and ridge showing). `-- --measure` prints these fractions per clip. Sounds: `tools/audio/crocodile_sounds.py`
  (`sounds/monsters/crocodile_{wake,att,die}.wav`: water surge + splash + hiss, jaw snap + hiss, bellow + splash).
  `--boss-texture` bakes only `crocodile_sobek.png` (Sobek: green-black hide, gold scutes, red eyes, a gold-and-lapis collar band
  painted round the neck at y 0.325-0.435, palette keys `collar`, `lapis`), `CROCODILE_TEX=sobek` (with `--bake`).
* `tools/blender/models/scorpion.py` - Egyptian deathstalker (Leiurus quinquestriatus), built like `scarab.py` / `rat.py` (faces +Y, rotA 180):
  rigid parts per bone; carapace with median and lateral eyes and chelicerae, seven overlapping tergites on a `meso` flex bone, eight
  two-bone IK legs (alternating tetrapod gait L1 R2 L3 R4 / R1 L2 R3 L4 with planted feet, two strides per walk clip), pedipalps by
  two-bone IK towards a wrist target with a hinged movable finger, and the metasoma (five segments + telson) as an FK chain whose segment
  directions are absolute angles in the body's mid plane, blended between the curled rest pose, the strike and the limp pose (limp joints
  sag and are laid onto the floor, as in `rat.py`). Per-frame floor fix from a numpy copy of the skinning (tail excluded), plus a lift if the
  sting would dip below the floor. Clips: walk 24 (the reference, about 1.95 long x 0.93 high x 1.23 wide), attack 24 (loops from walk
  frame 0: claws grab and pinch, tail cocks, strikes over the head with the sting stabbing down between the claws 0.18 in front of the
  carapace, recoil; the build prints the sting tip per frame), die 30 (convulses, rolls onto its side belly up, legs curl in, tail limp on
  the floor). Bakes `scorpion.png` and `scorpion_giant.png` (black-brown, a carnelian sheen on the claws and the tail, the telson
  darker red) on the same UVs; `SCORPION_TEX=giant` shows the giant one in review renders. `--boss-texture` bakes only
  `scorpion_queen.png` (Serket: pale gold, lapis-blue joints, rims and claw tips, a gold sting), `SCORPION_TEX=queen`. Sounds: `tools/audio/scorpion_sounds.py` (`sounds/monsters/scorpion_{att,die}.wav`: claw clacks + tail hiss and whip,
  dry chitin rattle and scraping legs).
* `tools/blender/models/egg_cluster.py` - the scorpion queen's egg cluster, a rooted "monster" (faces -Y, rotA 0; round): 19
  leathery eggs (cream-amber, a net of dark red veins) in three rings and a top pair, tilted outwards, glued with resin blobs on a
  low sand mound. Each egg is rigid on its own bone (pointing up from its centre); a resin blob is weighted half to each of its two
  eggs. Clips: move 24 (the reference, loops: the eggs swell out of step, two twitch; 0.50 high x 0.88 wide), attack 16 (loops: a
  heave runs down from the top, the root swells; up to 0.52 high), die 20 (once: the top eggs swell and burst first, every egg
  collapses into a flat husk on the mound). Every frame stays at or above the floor (printed per clip).
  Sounds: `tools/audio/egg_cluster_sounds.py` (`sounds/monsters/egg_cluster_{att,die}.wav`: squelches and a shell crack; bursts,
  splatter, drips).
* `tools/blender/models/cobra.py` - Egyptian cobra (Naja haje), faces +Y, rotA 180: one tube along 57 spine joints (a bone each,
  the vertices blend between the two joints round them; rest frames are plain translations), posed by heading, elevation and roll
  along the body integrated from the tail (the length never changes, as in `worm.py`), plus a hood amount that spreads the neck
  joints sideways and flattens them (bone scale). Rigid head with eyes and fangs, hinged lower jaw, forked tongue sliding out
  of the mouth. Every frame the base joint of the raised front stays at y 0.15 (blended towards the coil's centre in the idle),
  lifted so the lowest vertex touches the floor (numpy copy of the skinning). Clips: move 24 (the reference: waves run back along
  the body on the floor, the front 1.05 raised in a column, the head level, hood half open; about 0.96 high x 2.19 long x 0.55
  wide, halfX 0.27, halfZ 1.10), attack 22 (loops from move frame 0: rears back with the hood spread, strikes forward and down
  with the mouth wide open, the head about 0.5 high, snaps, recoils), die 30 (writhes, the front falls to its side and rolls
  belly up), idle 32 (`_idle`: coiled flat, the tail inside, the head resting on the outer coil, hood closed, breath swell and a
  tongue flick; 0.23 high), rise 24 (`_rise`, once: from idle frame 0 the body unwinds, the front rears up and the hood spreads; last
  frame = move frame 0), spit 18 (`_spit`, once: hood flared, the head draws back and jerks forward with the mouth open; the venom
  leaves at frame 7 (`SPIT_RELEASE`), the mouth then 0.84 high (0.87 of the move clip's height) and 1.20 forward of the reference
  centre; last frame = move frame 0). `-- --measure` prints every clip's frame 0 extents and the mouth per spit frame.
  Bakes `cobra.png` and `cobra_giant.png` (black-necked: near black, pale cream throat band, amber eyes) on the same UVs;
  `COBRA_TEX=giant` shows the giant one in review renders. `--boss-texture` bakes only `cobra_apep.png` (Apep: red-black, gold-green
  scale edges, yellow eyes, a dark red hood with two pale eye-spot rings; palette keys `edge`, `hood`, `spot`), `COBRA_TEX=apep`.
  Sounds: `tools/audio/cobra_sounds.py` (`sounds/monsters/cobra_{wake,att,spit,die}.wav`: swelling hiss; hiss, swish and jaw
  snap; hiss and a wet 'pff'; gasping hiss, thud of the body, last slither).
* `tools/blender/models/plant.py` - static monster example: lathed jar, FK bone chains (stalk, vines) with per-bone Euler
  angles from pose parameters, hinged petals, poses eased off by bisection so nothing sinks through the floor.
* `tools/blender/models/decor.py` - fifteen static corridor props (web, pottery, canopic jars, rubble, sand drift, skeleton,
  brazier, lamp (offerings: clay oil lamp), scrolls, ushabti; broken statues: `cat` (Bastet on a chipped plinth, an ear and a
  toppled figurine on the floor), `jackal` (Anubis couchant on a black and gold shrine), `osiris` (toppled, the shins still on the
  base, atef crown rolled off), four of Thoth, one pick with a variant at even odds: `thoth_ibis_standing` (striding, palette and pen, beak snapped, moon crown on the floor), `thoth_ibis_seated` (enthroned, palette on the knees), `thoth_baboon_seated` (squatting, dark sandstone, crown fallen), `thoth_baboon_standing` (forepaws raised, one broken off); `sarcophagus` (a commoner's box
  coffin, lid cracked in two, its mummy lying in front)) plus the wall torch (`decor_torch`, placed by `scatterTorches` in `src/world/decor_scatter.cpp`, up to one per
  5 cells of a row) in tile units, lighting baked into the texture (sun from the camera side + AO),
  drawn textured only (no Centrify). `-- --export` writes `models/decorations/decor_<name>.md3` + `textures/decorations/decor_<name>.png`;
  `--review out.png` renders a line-up, `--only web,sand` limits the build; extents are printed (engine jitter table `DECOR_JITTER` in
  `src/world/decor_scatter.cpp`). Placement: `scatterDecor` in `src/world/decor_scatter.cpp` (30% of walkable empty floor cells, seeded by the level file name), from the
  decoration tiers the level's depth unlocks (`DECOR_TIERS` in `src/world/decor.h`: cave, worked tunnel, tomb, temple).
  In-game check of the statues and the sarcophagus: `make test SCENARIO=tests/scenarios/statues.txt` (puts them with the
  scenario's `prop` command); of the tiers: `tests/scenarios/decor_depth.txt`.
  `coffin` (the mummy's, not scattered: the engine puts it on mummy spawn tiles; no `PROP_SCALE`, exact tile units): empty open box,
  interior x +-0.30, y -0.22..-0.06, inner floor z 0.012, walls 0.016 thick, rim z 0.08, headrest and wedjat eyes at the head end (-X),
  lid leaning against the wall behind, torn wrappings over the front rim. `mummy.py` `COFFIN` uses the same numbers.
* `tools/blender/models/ladder.py` - ladder pieces for `Ladder` cells, same tile units and baked lighting as `decor.py`: two styles
  (`wood`: acacia poles with rope-lashed rungs; `vine`: two twisted lianas with thin vines as holds), each with three
  interchangeable middle pieces `a`/`b`/`c` plus `top` (wood: roped to a beam between the side walls; vine: roots creeping along
  under the ceiling) and `bottom` (on the floor). Rails sit at x = +-0.1, y = -0.04 and match at z = 0 / 1, so pieces stack in any
  order; holds every 1/12 tile (for the climbing animations to come). `-- --export` writes `models/ladders/ladder_<style>_<piece>.md3` +
  `textures/ladders/ladder_<style>_<piece>.png`; `--review out.png [--view z,scale]` renders both styles as stacked shafts.
  Placement: `scatterLadders` in `src/world/decor_scatter.cpp` (one style per shaft, no middle piece twice in a row, wooden pieces mirrored at random;
  lianas are never mirrored, their twist would kink at the seams). Tables: `LADDER_*` in `src/world/decor.h`.
* `tools/blender/models/mechanism.py` - level mechanics, same tile units and baked lighting as `decor.py`: `key` (upright ankh key,
  oval gem in the loop), `gate` (bronze portcullis across the corridor: plane y-z at x = 0, 0.21 thick, full tile depth and height,
  stone lintel and back stile, lock cartouche on the front face, gem medallion on both long sides, spiked feet; slides up into the
  ceiling as a whole), `lever_base` (wall plate at z 0.23..0.57 with slot, bearing boss and gem) + `lever_handle` (origin = pivot, points
  +Z; pivot at `LEVER_PIVOT` = (0, -0.05, 0.42) in the base frame, swung +-35 deg around the depth axis), `rock` (faceted sandstone
  boulder ~0.38 across) and `ceiling_crack` (loose stones sagging out of the ceiling at z = 1, dark gaps, sand trickle), `pressure_plate`
  (a dart trap's sandstone slab in the floor, a dark gap round it, a cobra carved on top; the engine sinks it while
  pressed), `dart_holes` (a panel of three bronze-rimmed holes in the back wall above the plate, at the darts' height).
  Gate, lever, crack, plate and holes use the decor frame (origin on the back wall, `drawDecorTile`'s transform); key and rock are centred on their own
  vertical axis with the lowest point at z = 0 (draw from the tile centre, like the old ankh item; the key spins around its shaft).
  Key, gate and lever_base have one texture per lock colour on the same UVs (`red` carnelian, `blue` lapis, `green` turquoise,
  `gold` yellow amber; only gems and painted accents differ). `-- --export` writes `models/mechanisms/<model>.md3` +
  `textures/mechanisms/<model>[_<colour>].png`; `--review out.png` (textured with `--bake` or `--export`) renders colour line-ups
  (`out.png`, `out_small.png`) and two corridor shots from the game camera (`out_corridor{1,2}.png`); `--only key,gate`.
* `tools/blender/models/items.py` - the weapons (club, dagger, short sword (`sword`), khopesh, epsilon and duckbill axes, mace, spear, self-bow (`bow`), composite bow, sling, throwing stick, javelin), the missiles in flight (arrow, sling stone), the six potion vessels, the treasure chest and the ten amulets
  (`amulet_<type>`, one per `AmuletType`, all its tiers share it: a short cord loop with a gold bail and a pendant in
  the x-z plane facing -Y; strength a jackal's fang, armor a bronze scarab, health a carnelian ib heart, poison a gold
  scorpion, traps the eye of Horus, blunt the djed pillar, slash the tyet knot, pierce the shen ring, regeneration a
  faience lotus, venom Wadjet's uraeus (a rearing cobra, hood to the front); their HUD icons in `tools/textures/hud_icons.py` match), real sizes in
  metres (the engine centres each and scales its largest dimension to 1, `Item::loadModel`). Weapons stand on +Z, grip at the
  bottom, flat faces in the x-z plane; the bow's back bulges to +x. The engine holds the weapon in the fist nearer the camera
  (`Player::Fist`: a fist vertex found in the idle clip, followed through every clip), at `WeaponMotion::grip` of its length
  up from the lowest point, tilted and swung per its `WeaponDef` (`ITEMS` in `src/world/items.cpp`).
  The bows (`bow`, `composite_bow`) are held upright by their grip and have `BOW_FRAMES` (8) frames, the draw: frame 0 at rest, then the string pulled back
  with an arrow on it (collapsed onto the nock in frame 0, so every frame has the same vertices); the engine picks the frame
  from the draw time (`Item::Draw(pose)`). Texture and UVs come from the fully drawn bow, the frames from shape keys
  (`bow_frames`). The missiles in flight are drawn in metres, not centred (`Dungeon::drawMissiles`): `arrow.md3` (tip at +Z),
  `sling_stone.md3`, and the throwing stick and javelin models a second time (loaded uncentred for the flight).
  The chest faces -Y (drawn at rotA 0), lid open to +Y, a heap of gold inside for the tile's item to stand in.
  The potion vessels (`potion_<model>`, `PotionModel` in `src/world/items.h`; each about the flask's 0.18 m height):
  `flask` (round glass flask, cork and cord: small health, small stamina), `lotus` (glass jar on a gold foot, a lotus
  flower of faience petals at the mouth, two handles: large health, large stamina), `pilgrim` (flat faience "New Year
  flask" with gold rings: might), `canopic` (alabaster jar, falcon head lid: armor), `cobra` (slim vial, a gold cobra
  coiled round it, the hood is the stopper: antidote), `ankh` (ankh-shaped vessel, the loop is the neck: life). A vessel
  has no texture of its own: each potion bakes one on its UVs (`POTIONS`: `textures/items/potion_<kind>.png`, the
  palette keys `liquid` / `liquid_dark` it overrides), so look-alikes differ by the liquid only. The liquid reaches the
  shoulder: a chest's gold heap hides the bottom. The engine loads each vessel once and gives every potion its own
  texture (`loadPotions` in `src/state/assets.cpp`, `Item::shareModel`); no tint on the model, `PotionDef::colour`
  tints the icons only. Albedo x AO textures (no baked light), 512 px.
  `-- --export` writes `models/items/<item>.md3` for every item in `ITEMS` (`{club,...,arrow,sling_stone,potion_flask,...,treasure_chest}`) + `textures/items/<same>.png` (a potion vessel: its potions' textures); `--review out.png`
  renders front and three-quarter line-ups (`out.png`, `out_34.png`); `--only club,bow`.
* `tools/blender/models/props.py` - gateway (`sphinx.md3`: doorway at the tile's left edge around the plasma portal quad of
  `Dungeon::Draw`, two Anubis jackals on shrine plinths; exits drawn turned 180 deg), the ankh shrine (gold ankh on a dais between
  four obelisks), the question mark over riddle gates, the spike trap (also the death trap at scale 40) and the teleporter gate
  (`columns.md3`: two papyrus columns on a threshold under an architrave with winged sun discs; the plasma quad of
  `Dungeon::drawTeleporterTile` fills the gap in the columns' centre plane, the model symmetric in depth so Centrify keeps that plane).
  Gateway, ankh and teleporter are exactly 1 tile in their largest dimension, so Centrify keeps tile units. Albedo x AO textures.
  `-- --export` writes `models/props/{sphinx,ankh,questionmark,columns}.md3`, `models/traps/spikes.md3` + `textures/props/<name>.png`, `textures/traps/spikes.png`;
  `--review out.png` as in `items.py`. In-game check: `make test SCENARIO=tests/scenarios/props.txt` (every item, prop and trap).
* `tools/textures/decals.py` - wall decal atlas `textures/decorations/decals.png` (RGBA, 4x4 cells of 256 px, loaded with mipmaps): cracks,
  vines, roots, seepage, hieroglyph panels, cartouche, eye of Horus, winged sun, papyrus, dry grass, creeper, moss. Cell order =
  `DECAL_DEFS` in `src/world/decor.h` (anchor: free / ceiling / floor, quad size). `python3 tools/textures/decals.py`.
  Placement: `scatterDecals` in `src/world/decor_scatter.cpp` (35% of cells with a visible back wall, one decal per cell).
* `tools/textures/surfaces.py` - cell surfaces in `textures/dungeon/` (RGB 512 px, mipmapped): 8 walls (painted plaster, worn,
  broken to stone; dressed stone, cracked, sand-drifted; rough rock, strata), 3 floors (slabs, sand, cracked), 3 ceilings (stars
  over paint, slabs over stone, rough rock), `rock.png` (solid cells, one image over 2x2 cells). Order = `*_STYLE_NAMES` in
  `src/world/decor.h`. Variants of a kind share their base and fade their own detail out at the edges, so they tile in any
  order. `python3 tools/textures/surfaces.py [out_dir] [--preview sheet.png]`. Placement: `scatterSurfaces` in `src/world/decor_scatter.cpp` (per row
  of open cells: rough rock, or dressed stone cut into painted / bare stretches; all rough in the cave tier, less and
  less deeper down, `ROUGH_PERCENT` / `PAINTED_PERCENT` in `decor.h`). In-game check:
  `make test SCENARIO=tests/scenarios/surfaces.txt`.
* Rebuild all game files of a model: `blender -b --python tools/blender/models/<name>.py -- --export`
  (writes `models/<category>/<name>{,_att,_die}.md3`, `textures/<category>/<name>.png`, saves `tools/blender/models/<name>.blend`).
  In live Blender (MCP): `exec(open(p).read(), g); g["build"](bake=False)` for quick iteration.
* Engine side: `src/graphics/animated_model.cpp` (loader), `src/graphics/textures.cpp` (PNG textures), model/texture paths in `src/world/monster_kinds.cpp` (`KINDS`) and `src/world/items.cpp` (`ITEMS`; the player in `game_state.cpp`).
* Lighting: `src/graphics/lighting.cpp` (GLSL per-pixel point lights over a dark ambient; player, torches, braziers, oil lamps;
  toon mode (F1) snaps the light to cel bands), `src/graphics/ink.cpp` (toon ink outlines: depth-based post pass,
  lines on silhouettes and creases of anything that writes depth) and `src/graphics/fire.cpp` (stateless fire particles). Flame origins per prop: `BRAZIER_FIRE`,
  `LAMP_FIRE`, `TORCH_FIRE` in `src/world/dungeon_render_decor.cpp`; keep them in sync with the geometry in `decor.py`.
* `tools/audio/weapon_sounds.py` - synthesizes `sounds/items/`: a swing and a hit per melee weapon (`<weapon>_swing`,
  `<weapon>_hit`, shared by the weapons alike: `axe_hit`, `mace_hit`), the bow's draw and release, the sling's whirl and
  release, a throw, the arrow in a body and in stone (`arrow_hit`, `arrow_wall`), a stone or stick on a body or off a wall
  (`stone_hit`, `stone_wall`). Wired in `ITEMS` (`src/world/items.cpp`) and `MISSILE_RULES`
  (`src/world/dungeon_rules.h`); a melee hit sound plays only when the swing hits a monster.
* `tools/audio/jump_sound.py` - synthesizes `sounds/characters/archeologist_jump.wav` (boot scuff, effort "hup", cloth whoosh; 16-bit PCM).
* `tools/audio/amulet_sound.py` - synthesizes `sounds/items/amulet.wav` (an amulet put on or taken off: cord rustle, beads and the pendant clinking).
* `build/model-viewer <file.md3> [seconds] [options]` (`make model-viewer`, or `make run-model-viewer ARGS="..."`) - check exported files in the real engine.
  Space cycles the model's clips, T its textures, L the lighting (flat, the game's with the player's light, toon with
  ink lines), + / - change the loop speed, left-drag turns the model. Screenshots in the game's renderer (texture
  filtering, int16 steps, `Centrify`, lighting), headless: `xvfb-run -a -s "-screen 0 1280x1024x24" build/model-viewer
  models/monsters/rat.md3 --shot /tmp/shots --frames 0,6,12 --yaw 90 --light toon --texture rat_giant --size 480x480`
  writes `<stem>_<frame>.png` and exits (`--frames` default all; `--pitch`). The model in the dungeon, among walls and
  torches: a scenario ([testing.md](testing.md)), e.g. `tests/scenarios/props.txt`.

## Format and engine conventions
* Models are Quake 3 MD3 (binary, int16 positions, 16-bit normals in every frame, <= 4096 verts per surface,
  exporter splits surfaces). Game space Y-up, counter-clockwise triangles, each file scaled to fill the int16 range
  (header name holds `;unit=`, which the loader applies so all files of a model share real units). Loader: `AnimatedModel::Load` in `src/graphics/animated_model.cpp`.
* Blender space: Z-up. Facing depends on the monster's `rotA` in `KINDS` (`src/world/monster_kinds.cpp`): Anubis (180) faces +Y, worm (0) faces -Y.
  Check the old model's facing before remodelling.
* The engine normalizes a monster by its reference clip's frame 0 (the walk-slot file `<name>.md3`; `_idle` for the mimic, `AMBUSH_CLIPS`): largest dimension -> 1, centred in x/z,
  min Y on the floor (`Centrify`); the attack and die files get the same transform (`Normalize`, `CharacterModel::Load`),
  so their frame 0 may differ. Keeping frame 0 the same pose in all three files still gives the smoothest switches.
  Single-file models (items, props) are centred on their own frame 0.
* Animation states (`ModelState`): Idle, Move, Attack, Die, Jump, Climb, Rise, one file per clip. The file list (`ClipFiles` in
  `src/entities/character_model.h`) names each file's suffix and whether it loops; its first file is the reference: required, normalizes all
  clips and stands in for a missing optional clip. `MONSTER_CLIPS`: `<name>.md3` Move, `_att` Attack, `_die` Die, optional `_idle` Idle, optional `_jump` Jump (plays once).
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
  `model-viewer` looks in `textures/<category>/` (the model's sub-directory under `models/`) for `<stem>.png`, then the
  base model's `<base>.png` (`anubis_att` -> `anubis`; the base is the shortest `_` cut with its own `.md3`), then the
  variants `<base>_*.png` (`anubis_boss`), which T cycles through.
* Monsters need three files: `<name>.md3` move (loops), `<name>_att.md3` attack (loops), `<name>_die.md3` die (plays once, holds last frame),
  plus an optional `<name>_idle.md3` (loops; the bat hanging on the ceiling). Monsters without it show the move clip when idle.
  The mimic (`AMBUSH_CLIPS`) requires `_idle` (the closed chest) and uses it as the reference clip.
  Optional `<name>_jump.md3` (plays once, holds the last frame; the giant rat's and giant scarab's leap) and `sounds/<category>/<name>_jump.wav`.
  The mummy (`ENTOMBED_CLIPS`) adds a required `_idle` (dormant in its coffin) and `_rise` (`ModelState::Rise`, suffix `_rise`, plays once:
  its wake clip, climbing out; the last frame is walk frame 0).
  Any frame count per file (Anubis 26, worm 32/32/40, scarab 24/26/32 + jump 10, plant 32/26/36, rat 24/24/30 + jump 10, bat 12/12/24 + idle 24, mimic 32/22/30 + idle 42, archeologist 32/20/30 + jump 10 + climb 24, mummy 24/22/30 + idle 32 + rise 26, crocodile 24/22/30 + idle 32, scorpion 24/24/30); engine plays ~14 fps. Loops: key frame N = frame 0, export 0..N-1.
* Textures: PNG (`bake_texture` in `common.py` saves with Blender `file_format="PNG"`, RGB), 1024x1024 for the remodelled monsters and player (the mimic 1024x512).
  Loaded by `Texture::LoadPNG` (`src/graphics/textures.cpp`, stb_image); an alpha channel is kept if present, rows are flipped so UV v=0 is the image bottom.
* Sizes: Anubis 8.2k tris ~1.5 MB/file, worm 6.4k tris ~1.7-2.1 MB/file, scarab 12.5k tris ~2.3-3.0 MB/file (jump 1.1 MB), plant 10.9k tris ~2.5-3.3 MB/file, rat 5.8k tris ~1.1-1.4 MB/file (jump 0.5 MB), bat 6.2k tris ~0.64-1.17 MB/file, mimic 12.2k tris ~2.3-4.2 MB/file, archeologist 7.3k tris ~1.1-1.7 MB/file (jump 0.6 MB), mummy 5.7k tris ~1.0-1.5 MB/file, crocodile 7.8k tris ~1.8-2.6 MB/file, scorpion 10.5k tris ~2.1-2.5 MB/file; items 1.2-4.5k tris 32-143 KB (arrow 0.6k tris 16 KB, bow 2.3k tris 8 frames 170 KB), ladder pieces 5-9.4k tris 156-294 KB, gateway 23k tris 615 KB, teleporter 11.7k tris 307 KB, other props 1.5-2.5k tris 37-89 KB; 87 model files in total.

## Status
Paths relative to `models/` and `textures/`. UI screens are in `textures/ui/`, dungeon wall textures in `textures/dungeon/`, `plasma.png` in `textures/effects/`.

| Model | Files | Texture | Status |
|---|---|---|---|
| Anubis, Anubis boss (monsters) | `monsters/anubis{,_att,_die}.md3` | `monsters/anubis.png`, `monsters/anubis_boss.png` | remodelled (the boss uses the same files with its own texture) |
| Worm (monster) | `monsters/worm{,_att,_die}.md3` | `monsters/worm.png` | remodelled (man-eating worm) |
| Scarab, giant scarab (monsters) | `monsters/scarab{,_att,_die,_jump}.md3` | `monsters/scarab.png`, `monsters/scarab_giant.png` | remodelled (golden Scarabaeus sacer; the giant scarab uses the same files with its own texture) |
| Rat, giant rat (monsters) | `monsters/rat{,_att,_die,_jump}.md3` | `monsters/rat.png`, `monsters/rat_giant.png` | new (tomb rat; the giant rat uses the same files with its own texture) |
| Bat, giant bat, vampire bat (monsters) | `monsters/bat{,_att,_die,_idle}.md3` | `monsters/bat.png`, `monsters/bat_giant.png`, `monsters/bat_vampire.png` | new (tomb bat; the giant bat and the vampire bat boss use the same files with their own textures) |
| Mimic (monster) | `monsters/mimic{,_att,_die,_idle}.md3` | `monsters/mimic.png` | new (treasure chest with fangs and tongue; idle = the chest item) |
| Mummy (monster) | `monsters/mummy{,_att,_die,_idle,_rise}.md3` | `monsters/mummy.png` | new (linen-wrapped corpse; dormant in its coffin, climbs out with `_rise`) |
| Crocodile (monster) | `monsters/crocodile{,_att,_die,_idle}.md3` | `monsters/crocodile.png` | new (Nile crocodile; idle = lying flat, lurks floating at the water surface) |
| Scorpion (monster) | `monsters/scorpion{,_att,_die}.md3` | `monsters/scorpion.png` | new (Egyptian deathstalker; the sting is the hit) |
| Plant (monster) | `monsters/plant{,_att,_die}.md3` | `monsters/plant.png` | remodelled (tomb lotus in a painted jar; walk file = idle) |
| Player | `characters/archeologist{,_walk,_die,_jump,_climb}.md3` | `characters/archeologist.png` | remodelled (archaeologist with fedora) |
| Gateway ("sphinx"), ankh, question mark | `props/{sphinx,ankh,questionmark}.md3` | `props/{sphinx,ankh,questionmark}.png` | remodelled (static, `props.py`) |
| Teleporter gate ("columns") | `props/columns.md3` | `props/columns.png` | remodelled (static, `props.py`; Door, gate type 5, plasma quad between the columns) |
| Ladders (2 styles x 5 pieces) | `ladders/ladder_<style>_<piece>.md3` | `ladders/ladder_<style>_<piece>.png` | new (static, `ladder.py`) |
| Items: the 13 weapons, arrow, sling stone, chest | `items/club.md3`, ..., `items/treasure_chest.md3` | `items/club.png`, ..., `items/treasure_chest.png` | remodelled (static, `items.py`; the bow has 8 draw frames) |
| Potions (6 vessels, 8 potions) | `items/potion_<model>.md3` | `items/potion_<kind>.png` | new (static, `items.py`; a texture per potion, [potion textures](plan/potion-textures.md)) |
| Amulets (10 types) | `items/amulet_<type>.md3` | `items/amulet_<type>.png` | new (static, `items.py`) |
| Spikes trap, death trap | `traps/spikes.md3` | `traps/spikes.png` | remodelled (static, `props.py`) |
| Corridor decorations (15 props) | `decorations/decor_<name>.md3` | `decorations/decor_<name>.png` | new (static, `decor.py`) |
| Mummy's coffin | `decorations/decor_coffin.md3` | `decorations/decor_coffin.png` | new (static, `decor.py`; only at mummy spawn tiles) |
| Wall torch | `decorations/decor_torch.md3` | `decorations/decor_torch.png` | new (static, `decor.py`); flame = `Fire::TORCH` particles |
| Keys (4 lock colours) | `mechanisms/key.md3` | `mechanisms/key_<colour>.png` | new (static, `mechanism.py`) |
| Key gate (4 lock colours) | `mechanisms/gate.md3` | `mechanisms/gate_<colour>.png` | new (static, `mechanism.py`) |
| Wall lever (4 lock colours) | `mechanisms/lever_base.md3`, `mechanisms/lever_handle.md3` | `mechanisms/lever_base_<colour>.png`, `mechanisms/lever_handle.png` | new (static, `mechanism.py`) |
| Falling rock, ceiling crack | `mechanisms/rock.md3`, `mechanisms/ceiling_crack.md3` | `mechanisms/rock.png`, `mechanisms/ceiling_crack.png` | new (static, `mechanism.py`) |
| Dart trap plate, dart holes | `mechanisms/pressure_plate.md3`, `mechanisms/dart_holes.md3` | `mechanisms/pressure_plate.png`, `mechanisms/dart_holes.png` | new (static, `mechanism.py`; the darts are the arrow model) |
| Wall decals (16) | - | `decorations/decals.png` | new (generated, `tools/textures/decals.py`) |
| Walls, floors, ceilings, rock (15) | - | `dungeon/<style>.png` | new (generated, `tools/textures/surfaces.py`) |
