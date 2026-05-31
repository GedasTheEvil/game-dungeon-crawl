# Dungeon Crawl: A game for Linux, Windows and possibly OSX

My bachelors degree work in 2011. Requirements to compile:

* Make
* GCC
* OpenGL
* GLUT
* SDL
* SDL Mixer

To compile the game executable simply run `make`.

## Code quality tools

- Format source files: `make format`
- Run static analysis (clang-tidy): `make tidy`

`clang-tidy` uses the project configuration from `.clang-tidy`.
