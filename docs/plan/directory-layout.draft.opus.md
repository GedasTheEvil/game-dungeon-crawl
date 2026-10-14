# Directory layout: media/, content/, bin/

Status: ready, needs nothing first. Big mechanical move: do it when no task worktree is open, the rename touches
most of the tree.

## Goal

A cleaner repo root. Whole directories move and keep their inner layout:

| Now                     | New                             |
|-------------------------|---------------------------------|
| `fonts/`                | `media/textures/fonts/`         |
| `textures/`             | `media/textures/`               |
| `sounds/`               | `media/sounds/`                 |
| `models/`               | `media/models/`                 |
| `levels/`               | `content/levels/`               |
| `inscriptions/`         | `content/inscriptions/`         |
| `riddles/`              | `content/riddles/`              |
| `game`, `levelcheck`, `levelgen`, `levelconvert` (root), `build/editor`, `build/model-viewer`, `build/unit` | `bin/` |

Example: `textures/ladders/ladder_vine_top.png` becomes `media/textures/ladders/ladder_vine_top.png`.
`build/` keeps the objects and libraries; only the linked executables go to `bin/`.

## Steps

1. `git mv` the directories (keeps history; one commit for the moves alone makes `git log --follow` and blame
   easy, the path fixes follow in a second commit).
2. Code: one place owns the roots. Add `kMediaDir`/`kContentDir` (or a small path helper) next to
   `src/state/assets.cpp`, which holds most asset paths (78 of ~100 in `src/`); the rest (`ui_draw`, `journal_view`,
   `item`, `end_screens`, `decor.h`, `campaign`, `menu`, `game_state`, `model_info`, `character_model`) use it too.
   No path literal with a top-level asset directory stays in `src/`.
3. Working directory: the game and tools read relative to the cwd and are started from the repo root today. Keep
   that (`bin/game` run from the root), and also resolve the root from the executable (`/proc/self/exe`, parent of
   `bin/`) so `bin/game` started from elsewhere still finds `media/`. Saves, `game.log` and `settings.ini` stay
   where they are now relative to that root.
4. Build: makefile outputs to `bin/`; `make clean` removes it; `.gitignore` gets `/bin/` instead of the root
   binaries, and the `sounds/music/soundtrack.ogg` line moves to `media/sounds/...`.
5. Callers of the binaries: `tools/run_scenarios.sh`, other `tools/*.sh`, the makefile targets
   (`paths`, `test`, `run-editor`, ...), `tests/`, Blender scripts in `tools/blender/` (they write into `models/`),
   `tools/textures`, `tools/audio`. The quick launchers `Play` and `ViewModel` stay in the root, unchanged in
   purpose: `cd` to the root as now, then start `bin/game` / `bin/model-viewer`.
6. Docs and agent text: `AGENTS.md` (`./levelcheck levels/lvl*` → `bin/levelcheck content/levels/lvl*`,
   `./game` in the scenario/xvfb rule), `docs/*.md`, `readme.md`, open drafts in `docs/plan/`. Don't rewrite
   `docs/plan/solved/`: they are history (55 of them name old paths).
7. Project skills (`.claude/skills/` in the repo, none yet) that name `./game`, `./levelcheck` or old asset paths
   get the new paths.

## Checks

* `git grep -nE '(^|[^/a-z_])(textures|sounds|models|fonts|levels|inscriptions|riddles)/' -- ':!docs/plan/solved'`
  only hits the new prefixed paths.
* `make check`, `make test`, `bin/levelcheck content/levels/lvl*` clean.
* Smoke: `xvfb-run -a timeout -k 5 20 bin/game --no-sound --level 1` loads textures, fonts, models, sounds; same
  started from another directory (`cd /tmp && xvfb-run ... ~/Apps/game-dungeon-crawl/bin/game ...`).
* Editor and model viewer start and show a level / a model.

Tooling-only change (no gameplay): moves straight to `docs/plan/solved/` once its checks pass.
