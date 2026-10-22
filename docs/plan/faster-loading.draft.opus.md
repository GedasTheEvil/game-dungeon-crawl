# Faster loading

Status: draft 2026-10-22, from the user. Ready: nothing needed first. Follow-up:
[loading-per-level](loading-per-level.draft.opus.md), only if startup is still slow after this one.

## Idea

The game loads every asset at start (`Assets::Load`, `src/state/assets.cpp`). Cut the time by removing wasted work
and using the idle cores, without changing what gets loaded.

## Measured (2026-10-22, level 1, `xvfb-run`, llvmpipe, 16 cores)

* Start to first level frame: 6.5 to 7 s. Max RSS 1.17 GB.
* Loading phases (strace open times, slowed by strace): monsters ~65 %, decor ~14 %, items ~11 %, mechanisms ~6 %,
  the rest (fonts, sounds, music, player) small. Monster `.md3` files: 139 of the 153 MB of models.
* perf, the whole run (~6.2 s CPU), the top entries:

| Share | Symbol | Cause |
|---|---|---|
| 13.7 % | `Md3Mesh::Load` | expands the indexed MD3 vertices into one entry per triangle corner, per frame |
| 12.4 % | `AnimatedModel::Compile` | merges those corners back into vertices: one string key per corner, over all frames |
| 4.5 % | `std::_Hash_bytes` | the hashing of those keys (plus malloc, memcmp) |
| 9.7 % | `sincosf` | `decodeNormal`: 3 trig calls per corner per frame; there are only 65536 normals |
| 13.6 % | `stbi__parse_zlib`, `stbi__create_png_image_raw` | PNG decoding, one file after another |

Everything runs on the main thread.

Measure again on your machine before and after; record the numbers in this plan:

```
printf 'seed 1\nlevel tests/levels/classic1\nquit\n' > /tmp/q.txt
/usr/bin/time -f "%e s, %M KB" xvfb-run -a timeout -k 5 90 bin/game --no-sound /tmp/q.txt
xvfb-run -a timeout -k 5 90 perf record -F 2000 -g -o /tmp/perf.data bin/game --no-sound /tmp/q.txt
perf report -i /tmp/perf.data --no-children --sort dso,symbol --stdio -g none
```

(`perf` needs `kernel.perf_event_paranoid` at 1 or lower. `perf report` with call graphs is very slow here; use
`-g none`.)

## Steps, cheapest first

Measure after each step. Stop when the remaining gain is not worth the work.

1. **Normal table.** `decodeNormal` (`src/entities/md3_mesh.cpp`) reads a 65536-entry table, built once (static,
   on first use). Same values as today.
2. **Indexed mesh.** Keep the MD3 indexing: per surface, the vertices once per frame, and the triangle indices
   offset by the surface's first vertex. `Compile` uploads these directly, without the corner keys and hash map.
   Today's merge also joins equal vertices across surfaces (seams). Dropping that only adds a few vertices; the
   picture stays the same.
   Callers that count by corner must move to vertices:
   * `AnimatedModel::VertexCount` returns `mesh.cornerCount` today. `PlayerView::findFists` / `Fist`
     (`src/entities/player_view.cpp`) loop over it and keep an index that they read in every frame (check how
     the player clip keeps its frames: `Compile(keepFrames)`).
   * `YRange`, `HalfXZ`, `getScale`, `Centrify`, `Translate`, `Scale` loop over `cornerCount * 3`. They work just
     as well over the vertices.
   * The uncompiled path of `AnimatedModel::Show` (`drawCorners` with `glDrawArrays`) needs the index array.
   * Kin sharing (`CharacterModel::Share`) and `Item::shareModel` must still share the parsed clips.
3. **Parallel parsing.** Decoding PNGs (`stbi_load`) and parsing MD3 files touches no GL: run it on worker threads
   (`std::async` or a small pool, at most `hardware_concurrency`). GL texture and buffer uploads stay on the main
   thread in a second pass. Split `Texture::LoadPNG` into decode and upload, and `AnimatedModel` into Load (CPU)
   and Compile (GL).
   The loading bar keeps moving: the main thread updates it while it uploads, or while it waits for the workers.
   Logs from the workers: the logger must be thread safe, or the workers return their errors to the main thread,
   which logs them.

Not part of this plan: other file formats (QOI, a binary mesh cache). Weigh them only if steps 1 to 3 are not
enough.

## Tests

* Unit (`tests/unit/`, no GL): `Md3Mesh` on a small `.md3` from `media/models` gives the same frame-0 box
  (`YRange`, `HalfXZ`) and the same frame count as before the change. The vertex count is at most the old corner
  count. The table normals equal `decodeNormal`'s old trig formula for every code.
* Scenario `smoke` still passes. Compare one screenshot of a monster and the player close up, before and after:
  same picture.
* `make check` clean. Write the before and after timings into this plan.

Does not change the game: once done and the checks pass, move it straight to solved (docs/plans.md, Solved).
