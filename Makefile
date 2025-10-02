CC=m68k-amigaos-gcc
VASM=vasmm68k_mot
VASMFLAGS=-Faout -devpac 
CFLAGS = -DDEMO_DEBUG -noixemul -Wall -O2 -I. -m68020
LDFLAGS = -noixemul
SOURCES=main.c textscroller.c textcontroller.c starlight/utils.c font.c \
		starlight/blob_controller.c starlight/graphics_controller.c \
		showlogo.c rotation/rotation.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=demo-1-gcc

all: c2p.o p2c.o $(SOURCES) $(EXECUTABLE) 

c2p.o: chunkyconverter/c2p.s
	$(VASM) $(VASMFLAGS) -o c2p.o chunkyconverter/c2p.s

p2c.o: chunkyconverter/p2c.s
	$(VASM) $(VASMFLAGS) -o p2c.o chunkyconverter/p2c.s

$(EXECUTABLE): $(OBJECTS) p2c.o c2p.o
	$(CC) $(LDFLAGS) $(OBJECTS) p2c.o c2p.o -o $@
	
clean: 
	rm *.o starlight/*.o starlight/*.uaem *.lnk *.info *.uaem \
		demo-1-gcc demo-1-sasc rotation/*.o
