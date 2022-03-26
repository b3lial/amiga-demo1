CC=m68k-amigaos-gcc
VASM=vasmm68k_mot
VASMFLAGS=-Faout -devpac 
CFLAGS = -D__far="" -DDEMO_DEBUG -Wall -O2 -I. -m68000
LDFLAGS = -noixemul 
SOURCES=main.c textscroller.c textcontroller.c starlight/utils.c font.c \
		starlight/blob_controller.c starlight/graphics_controller.c showlogo.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=demo-1-gcc

all: example.o $(SOURCES) $(EXECUTABLE) 

example.o: example.s
	$(VASM) $(VASMFLAGS) -o example.o example.s

$(EXECUTABLE): $(OBJECTS) example.o
	$(CC) $(LDFLAGS) $(OBJECTS) example.o -o $@
	
clean: 
	rm *.o starlight/*.o starlight/*.uaem *.lnk *.info *.uaem \
		demo-1-gcc demo-1-sasc
