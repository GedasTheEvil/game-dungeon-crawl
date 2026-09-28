##DungeonCrawl by Gedas The Evil
CXX=g++
RM=rm -f
CXXFLAGS=-Wall -Wextra -pedantic -Wold-style-cast -O3 -march=native -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -MMD -MP
TIDY_CPPFLAGS=-I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
LDFLAGS= -lX11  -lglut -lGL -lGLU -lm -ldl -L/usr/X11R6/lib -lSDL_mixer  -lSDL

SOURCES=$(sort $(wildcard src/*/*.cpp))
BUILD=build
OBJECTS=$(SOURCES:%.cpp=$(BUILD)/%.o)
DEPS=$(OBJECTS:.o=.d)

EXECUTABLE=game

# Level editor, runs from the repo root. Shares the game's texture, font, UI and level code.
EDITOR_SOURCES=$(wildcard tools/editor/*.cpp)
EDITOR_OBJECTS=$(EDITOR_SOURCES:%.cpp=$(BUILD)/%.o) $(addprefix $(BUILD)/src/, graphics/textures.o graphics/font.o \
	core/logger.o ui/ui_draw.o world/level.o world/level_check.o)
EDITOR=$(BUILD)/editor

# MD3 model viewer, runs from the repo root.
VIEWER_OBJECTS=$(BUILD)/tools/model-viewer/viewer.o $(addprefix $(BUILD)/src/, graphics/animated_model.o graphics/textures.o \
	graphics/hud.o graphics/font.o core/timer.o core/logger.o)
VIEWER=$(BUILD)/model-viewer

CLANG_TIDY?=clang-tidy
TIDY_JOBS?=$(shell nproc)

# Level tools (no GL): levelcheck validates and ranks levels, levelgen writes random ones. See docs/levels.md.
LEVEL_SOURCES=src/world/level.cpp src/world/level_check.cpp src/world/level_gen.cpp
LEVEL_TOOLS=levelcheck levelgen

.PHONY: all clean format tidy editor run-editor model-viewer run-model-viewer test level-tools

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

level-tools: $(LEVEL_TOOLS)

levelcheck: tools/level/levelcheck.cpp $(LEVEL_SOURCES) src/world/level.h src/world/level_check.h
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -Wold-style-cast -O2 tools/level/levelcheck.cpp $(LEVEL_SOURCES) -o $@

levelgen: tools/level/levelgen.cpp $(LEVEL_SOURCES) src/world/level.h src/world/level_check.h src/world/level_gen.h
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -Wold-style-cast -O2 tools/level/levelgen.cpp $(LEVEL_SOURCES) -o $@

clean:
	rm -rf $(BUILD) $(EXECUTABLE) $(LEVEL_TOOLS)

format:
	clang-format -i src/*/*.h src/*/*.cpp tools/level/*.cpp tools/editor/*.h tools/editor/*.cpp tools/model-viewer/*.cpp

tidy-fix:
	$(CLANG_TIDY) $(SOURCES) $(EDITOR_SOURCES) --fix -- $(TIDY_CPPFLAGS)

# One clang-tidy per file, TIDY_JOBS at a time (a header's warnings show once per file that includes it), without
# clang's "N warnings generated." lines (they count the warnings filtered out, e.g. in system headers).
# tidy-fix stays one process, so a fix in a shared header is applied once.
tidy:
	printf '%s\n' $(SOURCES) $(EDITOR_SOURCES) | xargs -P $(TIDY_JOBS) -I{} $(CLANG_TIDY) --quiet {} -- $(TIDY_CPPFLAGS) 2>&1 \
		| sed '/^[0-9]* warnings\? generated\.$$/d'

editor: $(EDITOR)

$(EDITOR): $(EDITOR_OBJECTS)
	$(CXX) $(EDITOR_OBJECTS) -o $@ $(LDFLAGS)

run-editor: $(EDITOR)
	./$(EDITOR)

model-viewer: $(VIEWER)

$(VIEWER): $(VIEWER_OBJECTS)
	$(CXX) $(VIEWER_OBJECTS) -o $@ $(LDFLAGS)

run-model-viewer: $(VIEWER)
	./$(VIEWER) $(ARGS)

# Scenario tests: `make test` runs tests/scenarios/*.txt, `make test SCENARIO=path` runs one. HEADLESS=0 shows the window.
test: $(EXECUTABLE)
	./tools/run_scenarios.sh $(SCENARIO)

-include $(DEPS) $(EDITOR_OBJECTS:.o=.d) $(VIEWER_OBJECTS:.o=.d)
