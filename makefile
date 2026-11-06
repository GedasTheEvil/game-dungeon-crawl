##DungeonCrawl by Gedas The Evil
CXX=g++
# RELEASE=1 (make release): a portable build in build/release, so it runs on an older CPU than this one's.
ifdef RELEASE
MARCH=-march=x86-64-v2
else
MARCH=-march=native
endif
RM=rm -f
# -Wno-missing-field-initializers: a table row in designated initializers leaves out what it does not need.
CXXFLAGS=-std=c++20 -Wall -Wextra -Wno-missing-field-initializers -pedantic -Wold-style-cast -O3 $(MARCH) -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -MMD -MP
TIDY_CPPFLAGS=-std=c++20 -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
# Game and tools link only what they use: GL for every window, SDL (audio) for the game alone.
GL_LIBS=-lX11 -lglut -lGL -lGLU -lm -ldl -L/usr/X11R6/lib
SDL_LIBS=-lSDL_mixer -lSDL

SOURCES=$(sort $(wildcard src/*/*.cpp))
ifdef RELEASE
BUILD=build/release
else
BUILD=build
endif
# The linked programs (plan directory-layout); build/ keeps the objects and libraries. Run them from the repo root.
ifdef RELEASE
BIN=build/release/bin
else
BIN=bin
endif
# Third-party implementations (stb), compiled without warnings and outside make tidy.
EXTERNAL_OBJECTS=$(BUILD)/external/stb/stb.o

# Libraries shared by the game and the tools (docs/plan/solved/layered-build.md), each on top of the ones before it:
# base <- level <- sim <- render <- the game and the tools. tools/check_layers.sh (make layers) keeps them apart:
# base, level and sim have no GL, no library has SDL or Game(), and each includes only its own headers and those of
# the libraries under it. The base library (the game clock, the log) is linked by everything that links one.
# One directory per library (plan src-dirs-by-layer): a new file there is part of it, no list to keep.
BASE_LIB_SOURCES=$(sort $(wildcard src/core/*.cpp))
BASE_LIB_HEADERS=$(wildcard src/core/*.h)
LEVEL_LIB_SOURCES=$(sort $(wildcard src/level/*.cpp))
LEVEL_LIB_HEADERS=$(wildcard src/level/*.h)
# The sim (docs/plan/solved/sim-library.md): the dungeon, the monsters and the player, their rules and the model facts
# they read, without GL. The dungeon's drawing (dungeon_render*.cpp) is in the game, in src/graphics/, with the views.
# render_config.h: the world's sizes in GL units (a tile is 40), the sim's lifts and depths, the drawing's frame.
SIM_LIB_SOURCES=$(sort $(wildcard src/sim/*.cpp))
SIM_LIB_HEADERS=$(wildcard src/sim/*.h)
RENDER_LIB_SOURCES=$(sort $(wildcard src/render/*.cpp))
RENDER_LIB_HEADERS=$(wildcard src/render/*.h)
BASE_LIB=$(BUILD)/libbase.a
LEVEL_LIB=$(BUILD)/liblevel.a
SIM_LIB=$(BUILD)/libsim.a
RENDER_LIB=$(BUILD)/librender.a
BASE_LIB_OBJECTS=$(BASE_LIB_SOURCES:%.cpp=$(BUILD)/%.o)
LEVEL_LIB_OBJECTS=$(LEVEL_LIB_SOURCES:%.cpp=$(BUILD)/%.o)
SIM_LIB_OBJECTS=$(SIM_LIB_SOURCES:%.cpp=$(BUILD)/%.o)
RENDER_LIB_OBJECTS=$(RENDER_LIB_SOURCES:%.cpp=$(BUILD)/%.o) $(EXTERNAL_OBJECTS)

EXECUTABLE=$(BIN)/game
APP_SOURCES=$(filter-out $(BASE_LIB_SOURCES) $(LEVEL_LIB_SOURCES) $(SIM_LIB_SOURCES) $(RENDER_LIB_SOURCES),$(SOURCES))
APP_OBJECTS=$(APP_SOURCES:%.cpp=$(BUILD)/%.o)

# Level editor, works from the repo root. Uses the game's texture, font, UI drawing and level code.
EDITOR_SOURCES=$(wildcard tools/editor/*.cpp)
EDITOR_OBJECTS=$(EDITOR_SOURCES:%.cpp=$(BUILD)/%.o)
EDITOR=$(BIN)/editor

# MD3 model viewer, works from the repo root.
VIEWER_SOURCES=$(wildcard tools/model-viewer/*.cpp)
VIEWER_OBJECTS=$(VIEWER_SOURCES:%.cpp=$(BUILD)/%.o)
VIEWER=$(BIN)/model-viewer

# Level tools (no GL): levelcheck validates and ranks levels, levelgen writes random ones, levelconvert rewrites
# old level files in the current format. See docs/levels.md.
LEVEL_TOOL_NAMES=levelcheck levelgen levelconvert
LEVEL_TOOLS=$(LEVEL_TOOL_NAMES:%=$(BIN)/%)
LEVEL_TOOL_SOURCES=$(LEVEL_TOOL_NAMES:%=tools/level/%.cpp)

# The monster balance sim pass (plan monster-balance): the unit tests' sim world, no GL. See docs/development.md.
BALANCE_SOURCES=tools/balance/balance.cpp
BALANCE=$(BIN)/balance

# Unit tests (doctest, external/doctest) of the library code, no GL context needed. tests/unit/main.cpp is the runner.
# stb (no GL) reads PNGs for the font sheet test.
UNIT_SOURCES=$(wildcard tests/unit/*.cpp)
UNIT_OBJECTS=$(UNIT_SOURCES:%.cpp=$(BUILD)/%.o)
UNIT=$(BIN)/unit

TIDY_SOURCES=$(SOURCES) $(EDITOR_SOURCES) $(VIEWER_SOURCES) $(LEVEL_TOOL_SOURCES) $(BALANCE_SOURCES)
DEPS=$(patsubst %.cpp,$(BUILD)/%.d,$(TIDY_SOURCES) $(UNIT_SOURCES)) $(EXTERNAL_OBJECTS:.o=.d)

CLANG_TIDY?=clang-tidy

# Every compile, link, clang-tidy and scenario game goes through one load gate (tools/load_gate.sh): a job cap, an
# idle-core, free-memory and free-swap floor, shared by all runs at once, also a make -j. Each heavy target ends with
# the gate's summary line (peaks, wall time). The build's stats start fresh with every make.
GATE=./tools/load_gate.sh run
# Parallel by default (plan check-speed): as many jobs as the gate's cap, which holds them back anyway. A serial build
# after a change to a widely included header (level/gameplay_config.h) took 5 minutes. `make -j1` builds serially.
# Lines of parallel jobs do not interleave (--output-sync=line), and a long run (make test) still prints as it goes.
ifeq ($(filter -j% j%,$(MAKEFLAGS)),)
MAKEFLAGS+=-j$(shell ./tools/load_gate.sh jobs) --output-sync=line
endif
export GATE_STATS:=$(BUILD)/.gate-build
$(shell rm -f $(GATE_STATS))
GATE_REPORT=@./tools/load_gate.sh report build

.PHONY: all release check clean format format-check layers tidy tidy-all tidy-fix editor run-editor model-viewer run-model-viewer test unit unit-serial paths level-tools

# The game and every tool, so a change to shared code cannot break a tool unseen.
all: $(EXECUTABLE) $(EDITOR) $(VIEWER) $(LEVEL_TOOLS) $(UNIT) $(BALANCE)
	$(GATE_REPORT)

$(EXECUTABLE): $(APP_OBJECTS) $(RENDER_LIB) $(SIM_LIB) $(LEVEL_LIB) $(BASE_LIB)
	@mkdir -p $(@D)
	$(GATE) $(CXX) $^ -o $@ $(GL_LIBS) $(SDL_LIBS)

# Each archive also depends on the makefile: a file moved into a library is older than the archive.
$(BASE_LIB): $(BASE_LIB_OBJECTS) makefile
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(LEVEL_LIB): $(LEVEL_LIB_OBJECTS) makefile
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(SIM_LIB): $(SIM_LIB_OBJECTS) makefile
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(RENDER_LIB): $(RENDER_LIB_OBJECTS) makefile
	$(RM) $@
	$(AR) rcs $@ $(filter %.o,$^)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(@D)
	$(GATE) $(CXX) $(CXXFLAGS) -c $< -o $@

# The doctest runner: doctest's own code, without our warnings.
$(BUILD)/tests/unit/main.o: tests/unit/main.cpp
	@mkdir -p $(@D)
	$(GATE) $(CXX) $(CXXFLAGS) -w -c $< -o $@

$(BUILD)/external/%.o: external/%.cpp
	@mkdir -p $(@D)
	$(GATE) $(CXX) $(CXXFLAGS) -w -c $< -o $@

# The build's version on the leaderboard (plan leaderboard): the git hash, "-dirty" with uncommitted changes. The stamp
# is rewritten only when it changes, so a new commit rebuilds the one file that records a win.
GAME_VERSION:=$(shell git describe --always --dirty 2>/dev/null || echo unknown)
VERSION_STAMP=$(BUILD)/version.txt
$(shell mkdir -p $(BUILD); [ "$$(cat $(VERSION_STAMP) 2>/dev/null)" = "$(GAME_VERSION)" ] || echo "$(GAME_VERSION)" > $(VERSION_STAMP))
$(BUILD)/src/ui/win_entry.o: $(VERSION_STAMP)
$(BUILD)/src/ui/win_entry.o: CXXFLAGS += -DGAME_VERSION=\"$(GAME_VERSION)\"

level-tools: $(LEVEL_TOOLS)

$(LEVEL_TOOLS): $(BIN)/%: $(BUILD)/tools/level/%.o $(LEVEL_LIB) $(BASE_LIB)
	@mkdir -p $(@D)
	$(GATE) $(CXX) $^ -o $@

$(BALANCE): $(BALANCE_SOURCES:%.cpp=$(BUILD)/%.o) $(SIM_LIB) $(LEVEL_LIB) $(BASE_LIB)
	@mkdir -p $(@D)
	$(GATE) $(CXX) $^ -o $@

# The release zip (plan release-zip, tools/release.sh): the portable game build, bin, media, content and Play.
release:
	./tools/release.sh $(ARGS)

clean:
	rm -rf $(BUILD) $(BIN)

FORMAT_FILES=$(wildcard src/*/*.h src/*/*.cpp tools/level/*.cpp tools/editor/*.h tools/editor/*.cpp \
	tools/model-viewer/*.h tools/model-viewer/*.cpp tools/balance/*.cpp tests/unit/*.cpp)

format:
	@clang-format -i $(FORMAT_FILES)

# Fails on a file `make format` would change, or one clang-format does not leave stable (flips on every run).
format-check:
	@./tools/check_format.sh $(FORMAT_FILES)

layers:
	@./tools/check_layers.sh base $(BASE_LIB_SOURCES) $(BASE_LIB_HEADERS) -- level $(LEVEL_LIB_SOURCES) $(LEVEL_LIB_HEADERS) -- sim $(SIM_LIB_SOURCES) $(SIM_LIB_HEADERS) -- render $(RENDER_LIB_SOURCES) $(RENDER_LIB_HEADERS)
	@./tools/check_sim.sh $(SIM_LIB_SOURCES) $(SIM_LIB_HEADERS)

tidy-fix: export GATE_STATS=$(BUILD)/.gate-tidy
tidy-fix:
	@mkdir -p $(BUILD)
	$(GATE) $(CLANG_TIDY) $(TIDY_SOURCES) --fix -- $(TIDY_CPPFLAGS); \
		code=$$?; ./tools/load_gate.sh report tidy-fix; exit $$code

# `make tidy`: clang-tidy on the sources changed since the merge base with master (and the ones including a changed
# header); `make tidy-all`: on every source. One clang-tidy per file, through the gate. Both fail on any warning: the
# tree is kept clean, so each warning is the run's own (tools/tidy.sh). tidy-fix stays one process, so a fix in a
# shared header is applied once.
tidy: export GATE_STATS=$(BUILD)/.gate-tidy
tidy: layers
	@mkdir -p $(BUILD) && rm -f $(GATE_STATS)
	@./tools/tidy.sh changed $(TIDY_SOURCES) -- $(TIDY_CPPFLAGS); code=$$?; ./tools/load_gate.sh report tidy; exit $$code

tidy-all: export GATE_STATS=$(BUILD)/.gate-tidy
tidy-all: layers
	@mkdir -p $(BUILD) && rm -f $(GATE_STATS)
	@./tools/tidy.sh all $(TIDY_SOURCES) -- $(TIDY_CPPFLAGS); code=$$?; ./tools/load_gate.sh report tidy; exit $$code

# `make check`: everything a change needs before its commit, in order. Formats the code (never by hand), then checks
# it is stable, then tidy on the changed sources, then the unit tests. Scenarios stay `make test`. Links to plans that
# moved are rewritten first (tools/plan.py), a link it cannot fix fails; then the plans' names and Status lines
# (plan.py lint) and the plan tool's own tests.
check: all
	@./tools/plan.py links
	@./tools/plan.py lint
	@python3 -I tools/plan_test.py
	@$(MAKE) --no-print-directory format
	@./tools/check_format.sh --changed $(FORMAT_FILES)
	@$(MAKE) --no-print-directory tidy
	@$(MAKE) --no-print-directory unit

editor: $(EDITOR)

$(EDITOR): $(EDITOR_OBJECTS) $(RENDER_LIB) $(SIM_LIB) $(LEVEL_LIB) $(BASE_LIB)
	@mkdir -p $(@D)
	$(GATE) $(CXX) $^ -o $@ $(GL_LIBS)

run-editor: $(EDITOR)
	$(EDITOR) $(LEVEL)

model-viewer: $(VIEWER)

$(VIEWER): $(VIEWER_OBJECTS) $(RENDER_LIB) $(SIM_LIB) $(LEVEL_LIB) $(BASE_LIB)
	@mkdir -p $(@D)
	$(GATE) $(CXX) $^ -o $@ $(GL_LIBS)

run-model-viewer: $(VIEWER)
	$(VIEWER) $(ARGS)

$(UNIT): $(UNIT_OBJECTS) $(SIM_LIB) $(LEVEL_LIB) $(BASE_LIB) $(EXTERNAL_OBJECTS)
	@mkdir -p $(@D)
	$(GATE) $(CXX) $^ -o $@

# `make unit`: the unit tests in parallel shards (tools/unit_shards.sh, plan check-speed); `make unit-serial` runs them
# in one process, for a debugger or doctest's own options (bin/unit --help).
unit: $(UNIT)
	$(GATE_REPORT)
	@./tools/unit_shards.sh $(UNIT)

unit-serial: $(UNIT)
	$(GATE_REPORT)
	$(UNIT)

# `make paths`: levelcheck's path through every campaign level, played in the game (checks the checker's movement
# model against the real physics; docs/levels.md).
paths: $(EXECUTABLE) $(BIN)/levelcheck
	$(GATE_REPORT)
	rm -rf tests/out/paths && mkdir -p tests/out/paths
	$(BIN)/levelcheck --quiet --script tests/out/paths content/levels/lvl* >/dev/null
	./tools/run_scenarios.sh tests/out/paths/*.txt

# `make test`: the unit tests, then tests/scenarios/*.txt; `make test SCENARIO=path` runs one scenario.
# HEADLESS=0 shows the window.
test: $(EXECUTABLE) unit
	./tools/run_scenarios.sh $(SCENARIO)

-include $(DEPS)
