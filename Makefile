CC=m68k-amigaos-gcc
VASM=vasmm68k_mot
VASMFLAGS=-Faout -devpac
CFLAGS = -DDEMO_DEBUG -noixemul -Wall -O2 -Isrc -m68020
LDFLAGS = -noixemul
SOURCES=src/main.c src/textscroller.c src/textcontroller.c src/starlight/utils.c \
		src/starlight/blob_controller.c src/starlight/graphics_controller.c \
		src/showlogo.c src/rotation/rotation.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=demo-1-gcc

all: c2p.o p2c.o $(SOURCES) $(EXECUTABLE)

c2p.o: src/chunkyconverter/c2p.s
	$(VASM) $(VASMFLAGS) -o c2p.o src/chunkyconverter/c2p.s

p2c.o: src/chunkyconverter/p2c.s
	$(VASM) $(VASMFLAGS) -o p2c.o src/chunkyconverter/p2c.s

$(EXECUTABLE): $(OBJECTS) p2c.o c2p.o
	$(CC) $(LDFLAGS) $(OBJECTS) p2c.o c2p.o -o $@

clean:
	rm -f *.o src/*.o src/starlight/*.o src/starlight/*.uaem src/rotation/*.o \
		*.lnk *.info *.uaem demo-1-gcc demo-1-sasc
