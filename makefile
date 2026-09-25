##DungeonCrawl by Gedas The Evil
CXX=g++
RM=rm -f
CXXFLAGS=-Wall -Wextra -pedantic -Wold-style-cast -O3 -march=native -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -MMD -MP
TIDY_CPPFLAGS=-I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT
LDFLAGS= -lX11  -lglut -lGL -lGLU -lm -ldl -L/usr/X11R6/lib -lSDL_mixer  -lSDL

SOURCES=\
	src/core/game.cpp src/core/timer.cpp src/core/sound.cpp src/core/service_locator.cpp src/core/logger.cpp \
	src/graphics/ani.cpp src/graphics/textures.cpp src/graphics/shader.cpp src/graphics/font.cpp src/graphics/particles.cpp src/graphics/fire.cpp src/graphics/lighting.cpp src/graphics/draw.cpp src/graphics/hud.cpp \
	src/entities/monster.cpp src/entities/player.cpp src/entities/monster_ai.cpp src/entities/item.cpp src/entities/trap.cpp \
	src/world/dungeon_base.cpp src/world/dungeon_io.cpp src/world/dungeon_monsters.cpp src/world/dungeon_render.cpp src/world/dungeon_decor.cpp src/world/dungeon_mechanisms.cpp src/world/loot.cpp src/world/level.cpp src/world/level_check.cpp src/world/level_gen.cpp src/world/campaign.cpp \
	src/ui/menu.cpp src/ui/inventory.cpp src/ui/stats.cpp src/ui/riddle.cpp src/ui/winlose.cpp \
	src/input/input.cpp \
	src/state/game_state.cpp \
	src/test/scenario.cpp

OBJECTS=$(SOURCES:.cpp=.o)
DEPS=$(OBJECTS:.o=.d)

EXECUTABLE=game
CLANG_TIDY?=clang-tidy

# Level tools (no GL): levelcheck validates and ranks levels, levelgen writes random ones. See docs/levels.md.
LEVEL_SOURCES=src/world/level.cpp src/world/level_check.cpp src/world/level_gen.cpp
LEVEL_TOOLS=levelcheck levelgen

.PHONY: all clean format tidy editor run-editor model-viewer run-model-viewer test level-tools

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

level-tools: $(LEVEL_TOOLS)

levelcheck: tools/level/levelcheck.cpp $(LEVEL_SOURCES) src/world/level.h src/world/level_check.h
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -Wold-style-cast -O2 tools/level/levelcheck.cpp $(LEVEL_SOURCES) -o $@

levelgen: tools/level/levelgen.cpp $(LEVEL_SOURCES) src/world/level.h src/world/level_check.h src/world/level_gen.h
	$(CXX) -std=c++17 -Wall -Wextra -pedantic -Wold-style-cast -O2 tools/level/levelgen.cpp $(LEVEL_SOURCES) -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(DEPS) $(LEVEL_TOOLS)

format:
	clang-format -i src/*/*.h src/*/*.cpp tools/level/*.cpp

tidy-fix:
	$(CLANG_TIDY) $(SOURCES) --fix -- $(TIDY_CPPFLAGS)

tidy:
	$(CLANG_TIDY) $(SOURCES) -- $(TIDY_CPPFLAGS)

editor:
	(cd DungeonEditor && ./make)

run-editor:
	(cd DungeonEditor && ./editor)

model-viewer:
	(cd ModelViewer && ./make)

run-model-viewer:
	./ModelViewer/viewer $(ARGS)

# Scenario tests: `make test` runs tests/scenarios/*.txt, `make test SCENARIO=path` runs one. HEADLESS=0 shows the window.
test: $(EXECUTABLE)
	./tools/run_scenarios.sh $(SCENARIO)

-include $(DEPS)
