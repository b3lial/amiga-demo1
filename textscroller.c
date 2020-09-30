#include "textscroller.h"

#include <stdio.h>
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
struct BitMap* ballBlob = NULL;
struct BitMap* ballBlobScreen = NULL;

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
    ballBlob = loadBlob("img/ball_207_207_3.RAW", VIEW_TEXTSCROLLER_DEPTH, 
            VIEW_TEXTSCROLLER_BALL_WIDTH, VIEW_TEXTSCROLLER_BALL_HEIGHT);
    if(ballBlob == NULL){
        writeLog("Error: Payload TextScroller, could not load ball blob\n");
        exitTextScroller();
        exitSystem(RETURN_ERROR); 
    }
    writeLogFS("TextScroller BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
            ballBlob->BytesPerRow, ballBlob->Rows, ballBlob->Flags, 
            ballBlob->pad);
    loadColorMap("img/ball_207_207_3.CMAP", colortable0, VIEW_TEXTSCROLLER_COLORS); 

    //Create View and ViewExtra memory structures
    initView(); 

    //Create Bitmap for ViewPort
    ballBlobScreen = createBitMap(VIEW_TEXTSCROLLER_DEPTH, VIEW_TEXTSCROLLER_WIDTH,
            VIEW_TEXTSCROLLER_HEIGHT);
    for(i=0; i<VIEW_TEXTSCROLLER_DEPTH; i++){
        BltClear(ballBlobScreen->Planes[i], 
                (ballBlobScreen->BytesPerRow) * (ballBlobScreen->Rows), 1);
    }
    writeLogFS("Screen BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
            ballBlobScreen->BytesPerRow, ballBlobScreen->Rows, 
            ballBlobScreen->Flags, ballBlobScreen->pad);
    
    //Add previously created BitMap to ViewPort so its shown on Screen
    addViewPort(ballBlobScreen, NULL, colortable0, VIEW_TEXTSCROLLER_COLORS, 
            0, 0, VIEW_TEXTSCROLLER_WIDTH, VIEW_TEXTSCROLLER_HEIGHT);

    //Copy Ball into ViewPort
    BltBitMap(ballBlob, 0, 0, ballBlobScreen, 60, 20, VIEW_TEXTSCROLLER_BALL_WIDTH, 
            VIEW_TEXTSCROLLER_BALL_HEIGHT, 0xC0, 0xff, 0);

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
    cleanBitMap(ballBlobScreen);
    cleanBitMap(ballBlob);
}
