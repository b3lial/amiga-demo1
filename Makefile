CC=m68k-amigaos-gcc
VASM=vasmm68k_mot
VASMFLAGS=-Faout -devpac
CFLAGS = -DDEMO_DEBUG -noixemul -Wall -O2 -Isrc -m68020
LDFLAGS = -noixemul
SOURCES=src/main.c src/effects/textscroller.c src/gfx/textcontroller.c src/utils/utils.c \
		src/gfx/stars.c src/gfx/graphicscontroller.c src/gfx/movementcontroller.c \
		src/effects/showlogo.c src/gfx/rotation.c src/gfx/zoom.c \
		src/effects/rotatingcube.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=demo-1-gcc

all: c2p.o p2c.o $(SOURCES) $(EXECUTABLE)

c2p.o: src/gfx/c2p.s
	$(VASM) $(VASMFLAGS) -o c2p.o src/gfx/c2p.s

p2c.o: src/gfx/p2c.s
	$(VASM) $(VASMFLAGS) -o p2c.o src/gfx/p2c.s

$(EXECUTABLE): $(OBJECTS) p2c.o c2p.o
	$(CC) $(LDFLAGS) $(OBJECTS) p2c.o c2p.o -o $@

clean:
	rm -f *.o src/*.o src/effects/*.o src/gfx/*.o src/utils/*.o src/rotation/*.o \
		*.lnk *.info *.uaem demo-1-gcc demo-1-sasc
