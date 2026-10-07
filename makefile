##DungeonCrawl by Gedas The Evil
CXX=g++
RM=rm -f
CXXFLAGS=-Wall -Wextra -pedantic -Wold-style-cast -O3 -march=native -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -MMD -MP
TIDY_CPPFLAGS=-I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
# Game and tools link only what they use: GL for every window, SDL (audio) for the game alone.
GL_LIBS=-lX11 -lglut -lGL -lGLU -lm -ldl -L/usr/X11R6/lib
SDL_LIBS=-lSDL_mixer -lSDL

SOURCES=$(sort $(wildcard src/*/*.cpp))
BUILD=build
# Third-party implementations (stb), compiled without warnings and outside make tidy.
EXTERNAL_OBJECTS=$(BUILD)/external/stb/stb.o

# Libraries shared by the game and the tools (docs/plan/solved/layered-build.md). tools/check_layers.sh (make layers)
# keeps them apart: level code has no GL, no library has SDL or Game(), and each includes only its own headers and
# the base library's. The base library (the game clock) sits under both and is linked by everything that links one.
BASE_LIB_SOURCES=src/core/timer.cpp
LEVEL_LIB_SOURCES=src/world/level.cpp src/world/level_check.cpp src/world/level_gen.cpp src/world/campaign.cpp \
	src/world/items.cpp src/world/item_bag.cpp src/world/quick_potion.cpp src/world/poison.cpp src/world/loot.cpp \
	src/world/progression.cpp src/world/tile_defs.cpp src/world/monster_kinds.cpp src/world/journal.cpp \
	src/world/view_window.cpp src/world/world_events.cpp src/world/decor_scatter.cpp src/input/bindings.cpp src/state/settings_ini.cpp \
	src/entities/player_stats.cpp
LEVEL_LIB_HEADERS=src/core/gameplay_config.h src/world/movement.h src/world/rng.h src/world/damage.h src/world/decor.h
RENDER_LIB_SOURCES=src/core/logger.cpp src/graphics/textures.cpp src/graphics/font.cpp \
	src/graphics/animated_model.cpp src/ui/ui_draw.cpp src/graphics/shader.cpp src/graphics/ink.cpp \
	src/graphics/lighting.cpp src/graphics/render_target.cpp src/graphics/motion_fx.cpp
BASE_LIB=$(BUILD)/libbase.a
LEVEL_LIB=$(BUILD)/liblevel.a
# The world and the entities: no Game(), no screens; the dungeon's drawing only in dungeon_render*.cpp (tools/check_sim.sh,
# make layers).
SIM_FILES=$(wildcard src/world/dungeon*.cpp src/world/dungeon*.h src/world/sim_links.h src/entities/*.cpp src/entities/*.h)
RENDER_LIB=$(BUILD)/librender.a
BASE_LIB_OBJECTS=$(BASE_LIB_SOURCES:%.cpp=$(BUILD)/%.o)
LEVEL_LIB_OBJECTS=$(LEVEL_LIB_SOURCES:%.cpp=$(BUILD)/%.o)
RENDER_LIB_OBJECTS=$(RENDER_LIB_SOURCES:%.cpp=$(BUILD)/%.o) $(EXTERNAL_OBJECTS)

EXECUTABLE=game
APP_SOURCES=$(filter-out $(BASE_LIB_SOURCES) $(LEVEL_LIB_SOURCES) $(RENDER_LIB_SOURCES),$(SOURCES))
APP_OBJECTS=$(APP_SOURCES:%.cpp=$(BUILD)/%.o)

# Level editor, runs from the repo root. Uses the game's texture, font, UI drawing and level code.
EDITOR_SOURCES=$(wildcard tools/editor/*.cpp)
EDITOR_OBJECTS=$(EDITOR_SOURCES:%.cpp=$(BUILD)/%.o)
EDITOR=$(BUILD)/editor

# MD3 model viewer, runs from the repo root.
VIEWER_SOURCES=$(wildcard tools/model-viewer/*.cpp)
VIEWER_OBJECTS=$(VIEWER_SOURCES:%.cpp=$(BUILD)/%.o)
VIEWER=$(BUILD)/model-viewer

# Level tools (no GL): levelcheck validates and ranks levels, levelgen writes random ones, levelconvert rewrites
# old level files in the current format. See docs/levels.md.
LEVEL_TOOLS=levelcheck levelgen levelconvert
LEVEL_TOOL_SOURCES=$(LEVEL_TOOLS:%=tools/level/%.cpp)

# Unit tests (doctest, external/doctest) of the library code, no GL context needed. tests/unit/main.cpp is the runner.
UNIT_SOURCES=$(wildcard tests/unit/*.cpp)
UNIT_OBJECTS=$(UNIT_SOURCES:%.cpp=$(BUILD)/%.o)
UNIT=$(BUILD)/unit

TIDY_SOURCES=$(SOURCES) $(EDITOR_SOURCES) $(VIEWER_SOURCES) $(LEVEL_TOOL_SOURCES)
DEPS=$(patsubst %.cpp,$(BUILD)/%.d,$(TIDY_SOURCES) $(UNIT_SOURCES)) $(EXTERNAL_OBJECTS:.o=.d)

CLANG_TIDY?=clang-tidy
# nproc - 4, at least 1: four cores stay free for the desktop
TIDY_JOBS?=$(shell n=$$(($$(nproc) - 4)); [ $$n -lt 1 ] && n=1; echo $$n)

.PHONY: all clean format format-check layers tidy tidy-fix editor run-editor model-viewer run-model-viewer test unit paths level-tools

# The game and every tool, so a change to shared code cannot break a tool unseen.
all: $(EXECUTABLE) $(EDITOR) $(VIEWER) $(LEVEL_TOOLS) $(UNIT)

$(EXECUTABLE): $(APP_OBJECTS) $(RENDER_LIB) $(LEVEL_LIB) $(BASE_LIB)
	$(CXX) $^ -o $@ $(GL_LIBS) $(SDL_LIBS)

# Each archive also depends on the makefile: a file moved into a library is older than the archive.
$(BASE_LIB): $(BASE_LIB_OBJECTS) makefile
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(LEVEL_LIB): $(LEVEL_LIB_OBJECTS) makefile
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(RENDER_LIB): $(RENDER_LIB_OBJECTS) makefile
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# The doctest runner: doctest's own code, without our warnings.
$(BUILD)/tests/unit/main.o: tests/unit/main.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -w -c $< -o $@

$(BUILD)/external/%.o: external/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -w -c $< -o $@

level-tools: $(LEVEL_TOOLS)

$(LEVEL_TOOLS): %: $(BUILD)/tools/level/%.o $(LEVEL_LIB) $(BASE_LIB)
	$(CXX) $^ -o $@

clean:
	rm -rf $(BUILD) $(EXECUTABLE) $(LEVEL_TOOLS)

FORMAT_FILES=$(wildcard src/*/*.h src/*/*.cpp tools/level/*.cpp tools/editor/*.h tools/editor/*.cpp \
	tools/model-viewer/*.h tools/model-viewer/*.cpp tests/unit/*.cpp)

format:
	clang-format -i $(FORMAT_FILES)

# Fails on a file `make format` would change, or one clang-format does not leave stable (flips on every run).
format-check:
	@./tools/check_format.sh $(FORMAT_FILES)

layers:
	./tools/check_layers.sh base $(BASE_LIB_SOURCES) -- level $(LEVEL_LIB_SOURCES) $(LEVEL_LIB_HEADERS) -- render $(RENDER_LIB_SOURCES)
	./tools/check_sim.sh $(SIM_FILES)

tidy-fix:
	$(CLANG_TIDY) $(TIDY_SOURCES) --fix -- $(TIDY_CPPFLAGS)

# One clang-tidy per file, TIDY_JOBS at a time (a header's warnings show once per file that includes it), without
# clang's "N warnings generated." lines (they count the warnings filtered out, e.g. in system headers).
# tidy-fix stays one process, so a fix in a shared header is applied once.
tidy: layers
	printf '%s\n' $(TIDY_SOURCES) | xargs -P $(TIDY_JOBS) -I{} $(CLANG_TIDY) --quiet {} -- $(TIDY_CPPFLAGS) 2>&1 \
		| sed '/^[0-9]* warnings\? generated\.$$/d'

editor: $(EDITOR)

$(EDITOR): $(EDITOR_OBJECTS) $(RENDER_LIB) $(LEVEL_LIB) $(BASE_LIB)
	$(CXX) $^ -o $@ $(GL_LIBS)

run-editor: $(EDITOR)
	./$(EDITOR)

model-viewer: $(VIEWER)

$(VIEWER): $(VIEWER_OBJECTS) $(RENDER_LIB) $(BASE_LIB)
	$(CXX) $^ -o $@ $(GL_LIBS)

run-model-viewer: $(VIEWER)
	./$(VIEWER) $(ARGS)

$(UNIT): $(UNIT_OBJECTS) $(LEVEL_LIB) $(BASE_LIB)
	$(CXX) $^ -o $@

unit: $(UNIT)
	./$(UNIT)

# `make paths`: levelcheck's path through every campaign level, played in the game (checks the checker's movement
# model against the real physics; docs/levels.md).
paths: $(EXECUTABLE) levelcheck
	rm -rf tests/out/paths && mkdir -p tests/out/paths
	./levelcheck --quiet --script tests/out/paths levels/lvl* >/dev/null
	./tools/run_scenarios.sh tests/out/paths/*.txt

# `make test`: the unit tests, then tests/scenarios/*.txt; `make test SCENARIO=path` runs one scenario.
# HEADLESS=0 shows the window.
test: $(EXECUTABLE) unit
	./tools/run_scenarios.sh $(SCENARIO)

-include $(DEPS)
