// Copyright 2021 Christian Ammann

#include "textscroller.h"

#include <clib/alib_protos.h>
#include <ctype.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <graphics/rastport.h>
#include <graphics/videocontrol.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"
#include "starlight/starlight.h"
#include "textcontroller.h"

WORD payloadTextScrollerState = TEXTSCROLLER_INIT;
struct BitMap *fontBlob = NULL;
struct BitMap *spaceBlob = NULL;
struct BitMap *textscrollerScreen = NULL;
ULONG *colortable1 = NULL;

WORD fsmTextScroller(void) {
    switch (payloadTextScrollerState) {
        case TEXTSCROLLER_INIT:
            initTextScroller();
            WaitTOF();
            initTextScrollEngine("hi there", 70, 60, TEXTSCROLLER_BLOB_FONT_DEPTH,
                    TEXTSCROLLER_VIEW_WIDTH);
            payloadTextScrollerState = TEXTSCROLLER_RUNNING;
            break;

        case TEXTSCROLLER_RUNNING:
            payloadTextScrollerState = executeTextScroller(TEXTSCROLLER_RUNNING, TEXTSCROLLER_SHUTDOWN);
            break;

        case TEXTSCROLLER_SHUTDOWN:
            exitTextScroller();
            return MODULE_FINISHED;
    }

    return MODULE_CONTINUE;
}

void initTextScroller(void) {
    UWORD colortable0[8];
    BYTE i = 0;
    writeLog("\n== initTextScroller() ==\n");

    // Load font bitmap and its colors
    writeLog("Load font bitmap and colors\n");
    fontBlob = loadBlob("img/charset_final.RAW", TEXTSCROLLER_BLOB_FONT_DEPTH,
                        TEXTSCROLLER_BLOB_FONT_WIDTH, TEXTSCROLLER_BLOB_FONT_HEIGHT);
    if (fontBlob == NULL) {
        writeLog("Error: Could not load font blob\n");
        exitTextScroller();
        exitSystem(RETURN_ERROR);
    }
    writeLogFS(
        "Font BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
        fontBlob->BytesPerRow, fontBlob->Rows, fontBlob->Flags,
        fontBlob->pad);
    loadColorMap("img/charset_final.CMAP", colortable0,
                 TEXTSCROLLER_BLOB_FONT_COLORS);

    // Load space background bitmap and colors
    writeLog("\nLoad space background bitmap and colors\n");
    spaceBlob = loadBlob("img/space4_320_125_8.RAW", TEXTSCROLLER_BLOB_SPACE_DEPTH,
                         TEXTSCROLLER_BLOB_SPACE_WIDTH, TEXTSCROLLER_BLOB_SPACE_HEIGHT);
    if (spaceBlob == NULL) {
        writeLog("Error: Could not load space blob\n");
        exitTextScroller();
        exitSystem(RETURN_ERROR);
    }
    writeLogFS(
        "Space Bitmap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
        spaceBlob->BytesPerRow, spaceBlob->Rows, spaceBlob->Flags,
        spaceBlob->pad);

    // Load space background color table
    colortable1 = AllocVec(COLORMAP32_BYTE_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS), MEMF_ANY);
    if (!colortable1) {
        writeLog("Error: Could not allocate memory for space bitmap color table\n");
        exitTextScroller();
        exitSystem(RETURN_ERROR);
    }
    writeLogFS("Allocated %d  bytes for space bitmap color table\n",
               COLORMAP32_BYTE_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS));
    loadColorMap32("img/space3_320_148_8.CMAP", colortable1, TEXTSCROLLER_BLOB_SPACE_COLORS);

    // Load Textscroller Screen Bitmap
    writeLog("\nLoad textscroller screen background bitmap\n");
    textscrollerScreen = createBitMap(TEXTSCROLLER_BLOB_FONT_DEPTH,
                                      TEXTSCROLLER_VIEW_WIDTH,
                                      TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT);
    for (i = 0; i < TEXTSCROLLER_BLOB_FONT_DEPTH; i++) {
        BltClear(textscrollerScreen->Planes[i],
                 (textscrollerScreen->BytesPerRow) * (textscrollerScreen->Rows),
                 1);
    }
    writeLogFS("TextScroller Screen BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
               textscrollerScreen->BytesPerRow, textscrollerScreen->Rows,
               textscrollerScreen->Flags, textscrollerScreen->pad);
    createStars(textscrollerScreen);

    // Create View and ViewExtra memory structures
    writeLog("\nCreate view\n");
    initView();

    // Add previously created BitMap for text display to ViewPort so its shown on Screen
    addViewPort(textscrollerScreen, NULL, colortable0, TEXTSCROLLER_BLOB_FONT_COLORS, FALSE,
                0, 0, TEXTSCROLLER_VIEW_WIDTH, TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT);

    // Add space background BitMap to ViewPort so its shown on Screen
    addViewPort(spaceBlob, NULL, colortable1,
                COLORMAP32_LONG_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS), TRUE,
                0, TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT + 2, TEXTSCROLLER_VIEW_WIDTH,
                TEXTSCROLLER_VIEW_SPACESECTION_HEIGHT);

    // clean the allocated memory of colortable, we dont need it anymore because we
    // have a proper copperlist now
    FreeVec(colortable1);
    writeLogFS("Freeing %d bytes of space bitmap color table\n",
               COLORMAP32_BYTE_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS));
    colortable1 = NULL;

    // Make View visible
    startView();
}

UWORD executeTextScroller(UWORD currentTCState, UWORD nextTCState) {
    if (mouseClick()) {
        terminateTextScrollEngine();
        return TEXTSCROLLER_SHUTDOWN;
    } else if (textScrollIsFinished()) {
        terminateTextScrollEngine();
        return nextTCState;
    } else {
        WaitTOF();
        executeTextScrollEngine();
        return currentTCState;
    }
}

void exitTextScroller(void) {
    writeLog("\n== exitTextScroller() ==\n");

    if (colortable1) {
        FreeVec(colortable1);
        colortable1 = NULL;
        writeLogFS("Freeing %d bytes of space bitmap color table\n",
                   COLORMAP32_BYTE_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS));
    }
    stopView();
    cleanBitMap(textscrollerScreen);
    cleanBitMap(fontBlob);
    cleanBitMap(spaceBlob);
    payloadTextScrollerState = TEXTSCROLLER_INIT;
}

void createStars(struct BitMap *bitmap) {
    BYTE i;
    UWORD x, y;

    struct RastPort rastPort = {0};
    InitRastPort(&rastPort);
    rastPort.BitMap = bitmap;

    SetAPen(&rastPort, 6);
    for (i = 0; i < 50; i++) {
        x = RangeRand(TEXTSCROLLER_VIEW_WIDTH);
        y = RangeRand(TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT);
        writeLogFS("random value x %d, y %d\n", x, y);
        WritePixel(&rastPort, x, y);
    }
}
