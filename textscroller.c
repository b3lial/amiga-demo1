#include "textscroller.h"

#include <stdio.h>
#include <ctype.h>

#include <exec/types.h>
#include <proto/graphics.h>
#include <proto/exec.h>

#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <graphics/videocontrol.h>
#include <dos/dos.h>

#include "starlight/starlight.h"
#include "main.h"

WORD payloadTextScrollerState = VIEW_TEXTSCROLLER_INIT;
struct BitMap* fontBlob = NULL;
struct BitMap* textscrollerScreen = NULL;

WORD fsmTextScroller(void){
    switch(payloadTextScrollerState){
        case VIEW_TEXTSCROLLER_INIT:
            initTextScroller();
            payloadTextScrollerState = VIEW_TEXTSCROLLER_RUNNING;
            break;

        case VIEW_TEXTSCROLLER_RUNNING:
            if(!executeTextScroller()){
                payloadTextScrollerState = VIEW_TEXTSCROLLER_SHUTDOWN;
            }
            break;

        case VIEW_TEXTSCROLLER_SHUTDOWN:
            exitTextScroller();
            return MODULE_FINISHED;
    }
    
    return MODULE_CONTINUE;
}

void initTextScroller(void){
    UWORD colortable0[] = { BLACK, RED, GREEN, BLUE, BLACK, RED, GREEN, BLUE };
    BYTE i = 0;
    writeLog("\n== Initialize View: TextScroller ==\n");

    //Load Charset Sprite and its Colors
    fontBlob = loadBlob("img/charset_final.RAW", VIEW_TEXTSCROLLER_DEPTH,
            VIEW_TEXTSCROLLER_FONT_WIDTH, VIEW_TEXTSCROLLER_FONT_HEIGHT);
    if(fontBlob == NULL){
        writeLog("Error: Payload TextScroller, could not load font blob\n");
        exitTextScroller();
        exitSystem(RETURN_ERROR); 
    }
    writeLogFS("TextScroller BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
            fontBlob->BytesPerRow, fontBlob->Rows, fontBlob->Flags, 
            fontBlob->pad);
    loadColorMap("img/charset_final.CMAP", colortable0, VIEW_TEXTSCROLLER_COLORS);

    //Create View and ViewExtra memory structures
    initView(); 

    //Create Bitmap for ViewPort
    textscrollerScreen = createBitMap(VIEW_TEXTSCROLLER_DEPTH, VIEW_TEXTSCROLLER_WIDTH,
            VIEW_TEXTSCROLLER_HEIGHT);
    for(i=0; i<VIEW_TEXTSCROLLER_DEPTH; i++){
        BltClear(textscrollerScreen->Planes[i], 
                (textscrollerScreen->BytesPerRow) * (textscrollerScreen->Rows), 1);
    }
    writeLogFS("Screen BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
            textscrollerScreen->BytesPerRow, textscrollerScreen->Rows, 
            textscrollerScreen->Flags, textscrollerScreen->pad);
    
    //Add previously created BitMap to ViewPort so its shown on Screen
    addViewPort(textscrollerScreen, NULL, colortable0, VIEW_TEXTSCROLLER_COLORS, 
            0, 0, VIEW_TEXTSCROLLER_WIDTH, VIEW_TEXTSCROLLER_HEIGHT);

    //Copy Text into ViewPort
    BltBitMap(fontBlob, 0, 0, textscrollerScreen, 10, 10, VIEW_TEXTSCROLLER_FONT_WIDTH,
            VIEW_TEXTSCROLLER_FONT_HEIGHT, 0xC0, 0xff, 0);

    //Make View visible
    startView();
}

BOOL executeTextScroller(void){
    if(mouseClick()){
        return FALSE;
    }
    else{
        return TRUE;
    }
}

void exitTextScroller(void){
    stopView();
    cleanBitMap(textscrollerScreen);
    cleanBitMap(fontBlob);
}

/**
 * Display text on screen using font provided in src bitmap
 */
void displayText(struct BitMap* src, struct BitMap* dest, char* text){
	BYTE len = strlen(text);
	BYTE i;

	for(i=0;i<len;i++){
		char currentChar = tolower(text[i]);
		if(currentChar < 'a' || currentChar > 'z'){
			writeLogFS("displayText: letter %s not supported, skipping\n", currentChar);
			continue;
		}
	}
}
