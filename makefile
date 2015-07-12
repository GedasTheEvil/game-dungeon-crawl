##DungeonCrawl by Gedas The Evil
CC=g++
RM=rm -f
CFLAGS=-c -Wall -pedantic -Xlinker -zmuldefs -o3 -march=native
CDFLAGS=-c -Wall -pedantic -Xlinker -zmuldefs
LDFLAGS= -lX11  -lglut -lGL -lGLU -lm -ldl -L/usr/X11R6/lib  `sdl-config --cflags --libs` -lSDL_mixer 

SOURCES=src/ANI.cpp src/game.cpp src/textures.cpp src/Dungeon.cpp src/draw.cpp src/input.cpp src/monster.cpp src/font.cpp src/sound.cpp src/monster_ai.cpp src/item.cpp src/inventory.cpp src/particles.cpp src/stats.cpp src/timer.cpp src/cashe.cpp src/trap.cpp src/riddle.cpp src/menu.cpp src/shader.cpp src/winloose.cpp

OBJECTS=$(SOURCES:.cpp=.o)

EXECUTABLE=game

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE) $(OBJECTSD)


