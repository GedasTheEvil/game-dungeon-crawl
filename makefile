##DungeonCrawl by Gedas The Evil
CXX=g++
RM=rm -f
CXXFLAGS=-Wall -Wextra -pedantic -O3 -march=native -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -MMD -MP
LDFLAGS= -lX11  -lglut -lGL -lGLU -lm -ldl -L/usr/X11R6/lib -lSDL_mixer  -lSDL

SOURCES=src/ANI.cpp src/game.cpp src/textures.cpp src/Dungeon.cpp src/draw.cpp src/input.cpp src/monster.cpp src/font.cpp src/sound.cpp src/monster_ai.cpp src/item.cpp src/inventory.cpp src/particles.cpp src/stats.cpp src/timer.cpp src/cashe.cpp src/trap.cpp src/riddle.cpp src/menu.cpp src/shader.cpp src/winloose.cpp

OBJECTS=$(SOURCES:.cpp=.o)
DEPS=$(OBJECTS:.o=.d)

EXECUTABLE=game

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(DEPS)

-include $(DEPS)
