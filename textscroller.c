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
struct BitMap *spaceBlob = NULL;
struct BitMap *textscrollerScreen = NULL;
ULONG *colortable1 = NULL;

WORD fsmTextScroller(void)
{
    //terminate effect on mouse click
    if (mouseClick())
    {
        resetTextController();
        payloadTextScrollerState = TEXTSCROLLER_SHUTDOWN;
    }

    // no mouse click -> execute state machine
    switch (payloadTextScrollerState)
    {

    // create view, load star field, planet earth and font
    case TEXTSCROLLER_INIT:
        initTextScroller();
        if (!initTextController(TEXTSCROLLER_BLOB_FONT_DEPTH,
                                TEXTSCROLLER_VIEW_WIDTH))
        {
            exitTextScroller();
            exitSystem(RETURN_ERROR);
        }
        WaitTOF();
        setStringTextController("hi there", 70, 60);
        payloadTextScrollerState = TEXTSCROLLER_MSG_1;
        break;

    // display "hi there"
    case TEXTSCROLLER_MSG_1:
        WaitTOF();
        executeTextController();
        if (isFinishedTextController())
        {
            payloadTextScrollerState = TEXTSCROLLER_MSG_2;
            resetTextController();
            setStringTextController("belial here", 20, 60);
        }
        break;

    // display "belial here"
    case TEXTSCROLLER_MSG_2:
        WaitTOF();
        executeTextController();
        if (isFinishedTextController())
        {
            payloadTextScrollerState = TEXTSCROLLER_SHUTDOWN;
            resetTextController();
        }
        break;

    // destroy view
    case TEXTSCROLLER_SHUTDOWN:
        exitTextController();
        exitTextScroller();
        return MODULE_FINISHED;
    }

    return MODULE_CONTINUE;
}

void initTextScroller(void)
{
    UWORD colortable0[8];
    BYTE i = 0;
    writeLog("\n== initTextScroller() ==\n");

    writeLog("\nLoad space background bitmap and colors\n");
    // Load space background bitmap
    spaceBlob = loadBlob("img/space4_320_125_8.RAW", TEXTSCROLLER_BLOB_SPACE_DEPTH,
                         TEXTSCROLLER_BLOB_SPACE_WIDTH, TEXTSCROLLER_BLOB_SPACE_HEIGHT);
    if (spaceBlob == NULL)
    {
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
    if (!colortable1)
    {
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
    for (i = 0; i < TEXTSCROLLER_BLOB_FONT_DEPTH; i++)
    {
        BltClear(textscrollerScreen->Planes[i],
                 (textscrollerScreen->BytesPerRow) * (textscrollerScreen->Rows),
                 1);
    }
    writeLogFS("TextScroller Screen BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
               textscrollerScreen->BytesPerRow, textscrollerScreen->Rows,
               textscrollerScreen->Flags, textscrollerScreen->pad);
    createStars(textscrollerScreen);

    // Load Textscroller color table
    loadColorMap("img/charset_final.CMAP", colortable0,
                 TEXTSCROLLER_BLOB_FONT_COLORS);

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

void exitTextScroller(void)
{
    writeLog("\n== exitTextScroller() ==\n");

    if (colortable1)
    {
        FreeVec(colortable1);
        colortable1 = NULL;
        writeLogFS("Freeing %d bytes of space bitmap color table\n",
                   COLORMAP32_BYTE_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS));
    }
    stopView();
    cleanBitMap(textscrollerScreen);
    cleanBitMap(spaceBlob);
    payloadTextScrollerState = TEXTSCROLLER_INIT;
}

void createStars(struct BitMap *bitmap)
{
    BYTE i;
    UWORD x, y;

    struct RastPort rastPort = {0};
    InitRastPort(&rastPort);
    rastPort.BitMap = bitmap;

    SetAPen(&rastPort, 6);
    for (i = 0; i < 50; i++)
    {
        x = RangeRand(TEXTSCROLLER_VIEW_WIDTH);
        y = RangeRand(TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT);
        writeLogFS("random value x %d, y %d\n", x, y);
        WritePixel(&rastPort, x, y);
    }
}
