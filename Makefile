CC=m68k-amigaos-gcc
CFLAGS = -D__far="" -DDEMO_DEBUG -Wall -O2 -I. 
LDFLAGS = -noixemul 
SOURCES=main.c textscroller.c textcontroller.c starlight/utils.c  starlight/graphics_controller.c \
		starlight/blob_controller.c starlight/bitmap_controller.c starlight/init.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=demo-1-gcc

all: $(SOURCES) $(EXECUTABLE) 

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@
	
clean: 
	rm *.o starlight/*.o starlight/*.uaem *.lnk *.info *.uaem \
		demo-1-gcc demo-1-sasc
