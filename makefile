##DungeonCrawl by Gedas The Evil
CXX=g++
RM=rm -f
CXXFLAGS=-Wall -Wextra -pedantic -O3 -march=native -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -MMD -MP
LDFLAGS= -lX11  -lglut -lGL -lGLU -lm -ldl -L/usr/X11R6/lib -lSDL_mixer  -lSDL

SOURCES=\
	src/core/game.cpp src/core/timer.cpp src/core/sound.cpp \
	src/graphics/ANI.cpp src/graphics/textures.cpp src/graphics/shader.cpp src/graphics/font.cpp src/graphics/particles.cpp src/graphics/draw.cpp src/graphics/hud.cpp \
	src/entities/monster.cpp src/entities/player.cpp src/entities/monster_ai.cpp src/entities/item.cpp src/entities/trap.cpp \
	src/world/Dungeon_base.cpp src/world/Dungeon_io.cpp src/world/Dungeon_monsters.cpp src/world/Dungeon_render.cpp \
	src/ui/menu.cpp src/ui/inventory.cpp src/ui/stats.cpp src/ui/riddle.cpp src/ui/winloose.cpp \
	src/input/input.cpp \
	src/state/cashe.cpp

OBJECTS=$(SOURCES:.cpp=.o)
DEPS=$(OBJECTS:.o=.d)

EXECUTABLE=game

.PHONY: all clean format

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(DEPS)

format:
	clang-format -i src/*/*.h src/*/*.cpp

editor:
	(cd DungeonEditor && ./make)

-include $(DEPS)
