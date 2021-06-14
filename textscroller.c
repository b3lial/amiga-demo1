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

#include "demo1.h"
#include "starlight/starlight.h"

WORD payloadTextScrollerState = TEXTSCROLLER_INIT;
struct BitMap *spaceBlob = NULL;
struct BitMap *textscrollerScreen = NULL;

// necessary to manipulate colors during runtime
ULONG colortable1[COLORMAP32_LONG_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS)];
UWORD colortable0[TEXTSCROLLER_BLOB_FONT_COLORS];
extern struct ViewData vd;

struct TextConfig *textList[TEXT_LIST_SIZE];
struct TextConfig text1;
struct TextConfig text2;
struct TextConfig text3;

WORD fsmTextScroller(void)
{
    //terminate effect on mouse click
    if (mouseClick())
    {
        resetTextController();
        payloadTextScrollerState = TEXTSCROLLER_FADE_WHITE;
    }

    // no mouse click -> execute state machine
    switch (payloadTextScrollerState)
    {
    case TEXTSCROLLER_INIT:
        // configure text scroll engine
        text1.currentText = "hi there";
        text1.charXPosDestination = MAX_CHAR_WIDTH + 70;
        text1.charYPosDestination = 40;
        textList[0] = &text1;
        textList[1] = NULL;
        WaitTOF();
        setStringsTextController(textList);
        payloadTextScrollerState = TEXTSCROLLER_MSG_1;
        break;

    // display "hi there"
    case TEXTSCROLLER_MSG_1:
        WaitTOF();
        executeTextController();
        if (isFinishedTextController())
        {
            // configure text scroll engine
            resetTextController();
            text1.currentText = "belial";
            text1.charXPosDestination = MAX_CHAR_WIDTH + 95;
            text1.charYPosDestination = 18;
            text2.currentText = "here";
            text2.charXPosDestination = MAX_CHAR_WIDTH + 110;
            text2.charYPosDestination = 70;
            textList[0] = &text1;
            textList[1] = &text2;
            textList[2] = NULL;
            pauseTimeTextController(180);
            WaitTOF();
            setStringsTextController(textList);
            payloadTextScrollerState = TEXTSCROLLER_MSG_2;
        }
        break;

    // display "belial here"
    case TEXTSCROLLER_MSG_2:
        WaitTOF();
        executeTextController();
        if (isFinishedTextController())
        {
            // configure text scroll engine
            resetTextController();
            text1.currentText = "presenting";
            text1.charXPosDestination = MAX_CHAR_WIDTH + 37;
            text1.charYPosDestination = 4;
            text2.currentText = "my";
            text2.charXPosDestination = MAX_CHAR_WIDTH + 133;
            text2.charYPosDestination = 44;
            text3.currentText = "first";
            text3.charXPosDestination = MAX_CHAR_WIDTH + 103;
            text3.charYPosDestination = 84;
            textList[0] = &text1;
            textList[1] = &text2;
            textList[2] = &text3;
            textList[3] = NULL;
            pauseTimeTextController(660);
            WaitTOF();
            setStringsTextController(textList);
            payloadTextScrollerState = TEXTSCROLLER_MSG_3;
        }
        break;

    // display "presenting my first"
    case TEXTSCROLLER_MSG_3:
        WaitTOF();
        executeTextController();
        if (isFinishedTextController())
        {
            // configure text scroll engine
            resetTextController();
            text1.currentText = "demo";
            text1.charXPosDestination = MAX_CHAR_WIDTH + 10;
            text1.charYPosDestination = 18;
            text2.currentText = "production";
            text2.charXPosDestination = MAX_CHAR_WIDTH + 45;
            text2.charYPosDestination = 70;
            textList[0] = &text1;
            textList[1] = &text2;
            textList[2] = NULL;
            pauseTimeTextController(300);
            WaitTOF();
            setStringsTextController(textList);
            payloadTextScrollerState = TEXTSCROLLER_MSG_4;
        }
        break;

    // display "demo production"
    case TEXTSCROLLER_MSG_4:
        WaitTOF();
        executeTextController();
        if (isFinishedTextController())
        {
            payloadTextScrollerState = TEXTSCROLLER_FADE_WHITE;
            resetTextController();
        }
        break;

    // display "demo production"
    case TEXTSCROLLER_FADE_WHITE:
        WaitTOF();
        fadeToWhite();
        if (hasFadeToWhiteFinished())
        {
            return MODULE_FINISHED;
        }
        break;
    }

    return MODULE_CONTINUE;
}

void initTextScroller(void)
{
    BYTE i = 0;
    writeLog("\n== initTextScroller() ==\n");

    for (; i < TEXT_LIST_SIZE; i++)
    {
        textList[i] = NULL;
    }

    writeLog("\nLoad space background bitmap and colors\n");
    // Load space background bitmap
    spaceBlob = loadBlob("img/space4_320_125_8.RAW", TEXTSCROLLER_BLOB_SPACE_DEPTH,
                         TEXTSCROLLER_BLOB_SPACE_WIDTH, TEXTSCROLLER_BLOB_SPACE_HEIGHT);
    if (spaceBlob == NULL)
    {
        writeLog("Error: Could not load space blob\n");
        exitStarlight();
        exitTextScroller();
        exit(RETURN_ERROR);
    }
    writeLogFS(
        "Space Bitmap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
        spaceBlob->BytesPerRow, spaceBlob->Rows, spaceBlob->Flags,
        spaceBlob->pad);

    // Load space background color table
    loadColorMap32("img/space3_320_148_8.CMAP", colortable1, TEXTSCROLLER_BLOB_SPACE_COLORS);

    // Load Textscroller Screen Bitmap
    writeLog("\nLoad textscroller screen background bitmap\n");
    textscrollerScreen = createBitMap(TEXTSCROLLER_BLOB_FONT_DEPTH,
                                      TEXTSCROLLER_VIEW_TEXTSECTION_WIDTH,
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
    createNewView();

    // Add previously created BitMap for text display to ViewPort so its shown on Screen
    addViewPort(textscrollerScreen, NULL, colortable0, TEXTSCROLLER_BLOB_FONT_COLORS, FALSE,
                0, 0, TEXTSCROLLER_VIEW_WIDTH, TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT,
                MAX_CHAR_WIDTH, 0);

    // Add space background BitMap to ViewPort so its shown on Screen
    addViewPort(spaceBlob, NULL, colortable1,
                COLORMAP32_LONG_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS), TRUE,
                0, TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT + 6, TEXTSCROLLER_VIEW_WIDTH,
                TEXTSCROLLER_VIEW_SPACESECTION_HEIGHT, 0, 0);

    // Make View visible
    startView();

    if (!initTextController(textscrollerScreen,
                            TEXTSCROLLER_BLOB_FONT_DEPTH,
                            TEXTSCROLLER_VIEW_TEXTSECTION_WIDTH))
    {
        exitStarlight();
        exitTextScroller();
        exit(RETURN_ERROR);
    }
}

void exitTextScroller(void)
{
    writeLog("\n== exitTextScroller() ==\n");
    exitTextController();

    if (textscrollerScreen)
    {
        cleanBitMap(textscrollerScreen);
        textscrollerScreen = NULL;
    }

    if (spaceBlob)
    {
        cleanBitMap(spaceBlob);
        spaceBlob = NULL;
    }

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
        x = RangeRand(TEXTSCROLLER_VIEW_TEXTSECTION_WIDTH);
        y = RangeRand(TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT);
        writeLogFS("random value x %d, y %d\n", x, y);
        WritePixel(&rastPort, x, y);
    }
}

void fadeToWhite(void)
{
    UWORD i = 0;
    UWORD incrementer;
    ULONG currentColor;

    // fade of text scroll area (viewPort[0])
    for (; i < TEXTSCROLLER_BLOB_FONT_COLORS; i++)
    {
        incrementer = 0;
        if ((colortable0[i] & 0x000f) != 0x000f)
        {
            incrementer |= 0x0001;
        }
        if ((colortable0[i] & 0x00f0) != 0x00f0)
        {
            incrementer |= 0x0010;
        }
        if ((colortable0[i] & 0x0f00) != 0x0f00)
        {
            incrementer |= 0x0100;
        }
        colortable0[i] += incrementer;
    }

    // fade of space background area (viewPort[1])
    for (i = 1; i < COLORMAP32_LONG_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS) - 1; i++)
    {
        currentColor = (colortable1[i] & 0x000000ff);
        if (currentColor == 0xff)
        {
            continue;
        }

        currentColor += 17;
        if (currentColor > 0x000000ff)
        {
            currentColor = 0x000000ff;
        }

        colortable1[i] = SPREAD(currentColor);
    }

    // calculated new color sets, now we can update copper and co
    LoadRGB4(&vd.viewPorts[0], colortable0, TEXTSCROLLER_BLOB_FONT_COLORS);
    LoadRGB32(&vd.viewPorts[1], colortable1);
}

BOOL hasFadeToWhiteFinished(void)
{
    UWORD i = 0;

    for (; i < TEXTSCROLLER_BLOB_FONT_COLORS; i++)
    {
        if ((colortable0[i] & 0x0fff) != 0x0fff)
        {
            return FALSE;
        }
    }

    for (i = 1; i < COLORMAP32_LONG_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS) - 1; i++)
    {
        if ((colortable1[i] & 0x000000ff) != 0xff)
        {
            return FALSE;
        }
    }
    return TRUE;
}
