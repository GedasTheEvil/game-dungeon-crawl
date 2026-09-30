# Stage 11: GL resources own themselves

Status: implemented 2026-09-30, verified in play 2026-09-30. Stage 11 of the
[code structure review](../code-structure-review.draft.md).

## Why

`Texture` never freed its GL texture and could be copied; `CharacterModel` and `Trap` kept copies of textures owned
elsewhere. `Font` freed its display lists but could be copied, so a copy would free them twice. `AnimatedModel`
never freed its display lists. Nothing ran at exit before Quit went through `glutLeaveMainLoop`, so it did not show;
now `main`'s cleanup runs.

## Done

* `Texture`: move-only, frees its texture in the destructor, and `LoadPNG` frees what it held before.
  `CharacterModel::Load` and `Player::Load` take the texture over (`Texture&&`); `Trap` points at the shared
  registry texture instead of copying it.
* `Font` and `AnimatedModel`: copy deleted; `AnimatedModel` frees its display lists.
* All of them free only while a GL context is current (`glContextCurrent`, `textures.h`): closing the window
  destroys the context before the cleanup runs.
* The compiler found no other copies (the game and every tool build). Screenshots pixel-identical; Quit exits with
  code 0 and the log ends the session; the editor and the viewer start.

## Not done

* Shader programs (`lighting.cpp`, `ink.cpp`), the ink FBO and the fire sprite are file-static and live for the
  whole run; freeing them at exit gains nothing.
* No VBOs: the models stay display lists (fixed-function GL throughout; a renderer rewrite is out of scope).
