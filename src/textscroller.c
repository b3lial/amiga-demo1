// Copyright 2021 Christian Ammann

#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/videocontrol.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/exec_protos.h>
#include <hardware/custom.h>

#include "textscroller.h"

#include "textcontroller.h"
#include "fsm_states.h"
#include "starlight/utils.h"
#include "starlight/graphics_controller.h"
#include "starlight/blob_controller.h"

__far extern struct Custom custom;

WORD payloadTextScrollerState = TEXTSCROLLER_INIT;

struct BitMap *spaceBlob = NULL;
struct BitMap *textscrollerScreen = NULL;

ULONG *colortable1 = NULL;
UWORD colortable0[TEXTSCROLLER_BLOB_FONT_COLORS];

struct TextConfig *textList[TEXT_LIST_SIZE];
struct TextConfig text1;
struct TextConfig text2;
struct TextConfig text3;

struct Screen *textScrollerScreen0;
struct Screen *textScrollerscreen1;

UWORD fsmTextScroller(void) {
    // terminate effect on mouse click
    if (mouseClick()) {
        resetTextController();
        payloadTextScrollerState = TEXTSCROLLER_FADE_WHITE;
    }

    // no mouse click -> execute state machine
    switch (payloadTextScrollerState) {
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
            if (isFinishedTextController()) {
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
            if (isFinishedTextController()) {
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
            if (isFinishedTextController()) {
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
            if (isFinishedTextController()) {
                payloadTextScrollerState = TEXTSCROLLER_FADE_WHITE;
                resetTextController();
            }
            break;

        // display "demo production"
        case TEXTSCROLLER_FADE_WHITE:
            WaitTOF();
            fadeToWhite();
            if (hasFadeToWhiteFinished()) {
                return FSM_TEXTSCROLLER_FINISHED;
            }
            break;
    }

    return FSM_TEXTSCROLLER;
}

UWORD initTextScroller(void) {
    BYTE i = 0;
    struct Rectangle starsClip;
    writeLog("== initTextScroller() ==\n\n");

    for (; i < TEXT_LIST_SIZE; i++) {
        textList[i] = NULL;
    }

    writeLog("Load space background bitmap and colors\n");
    // Load space background bitmap
    spaceBlob = loadBlob("img/space4_320_125_8.RAW", TEXTSCROLLER_BLOB_SPACE_DEPTH,
                         TEXTSCROLLER_BLOB_SPACE_WIDTH, TEXTSCROLLER_BLOB_SPACE_HEIGHT);
    if (spaceBlob == NULL) {
        writeLog("Error: Could not load space blob\n");
        goto __exit_init_scroller;
    }
    writeLogFS(
        "Space Bitmap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
        spaceBlob->BytesPerRow, spaceBlob->Rows, spaceBlob->Flags,
        spaceBlob->pad);

    // Load space background color table
    colortable1 = AllocVec(COLORMAP32_BYTE_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS), MEMF_ANY);
    if (!colortable1) {
        writeLog("Error: Could not allocate memory for space bitmap color table\n");
        goto __exit_init_scroller;
    }
    writeLogFS("Allocated %d  bytes for space bitmap color table\n",
               COLORMAP32_BYTE_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS));
    loadColorMap32("img/space3_320_148_8.CMAP", colortable1, TEXTSCROLLER_BLOB_SPACE_COLORS);

    // Load Textscroller Screen Bitmap
    textscrollerScreen = AllocBitMap(TEXTSCROLLER_VIEW_TEXTSECTION_WIDTH,
                                     TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT, TEXTSCROLLER_BLOB_FONT_DEPTH,
                                     BMF_CLEAR | BMF_DISPLAYABLE, NULL);
    writeLogFS("TextScroller Screen BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
               textscrollerScreen->BytesPerRow, textscrollerScreen->Rows,
               textscrollerScreen->Flags, textscrollerScreen->pad);

    // Load Textscroller color table
    loadColorMap("img/charset_final.CMAP", colortable0,
                 TEXTSCROLLER_BLOB_FONT_COLORS);

    writeLog("Create two screens\n");
    // text scroller section screen
    starsClip.MinX = 0;
    starsClip.MinY = 0;
    starsClip.MaxX = TEXTSCROLLER_VIEW_WIDTH;
    starsClip.MaxY = TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT;
    textScrollerScreen0 = createScreen(textscrollerScreen, TRUE,
                                       -MAX_CHAR_WIDTH, 0,
                                       TEXTSCROLLER_VIEW_TEXTSECTION_WIDTH,
                                       TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT,
                                       TEXTSCROLLER_BLOB_FONT_DEPTH, &starsClip);
    if (!textScrollerScreen0) {
        writeLog("Error: Could not allocate memory for text scroller screen\n");
        goto __exit_init_scroller;
    }
    LoadRGB4(&textScrollerScreen0->ViewPort, colortable0, TEXTSCROLLER_BLOB_FONT_COLORS);

    initStars(50, TEXTSCROLLER_VIEW_TEXTSECTION_WIDTH, TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT);
    createStars(&textScrollerScreen0->RastPort, 6, 50, TEXTSCROLLER_VIEW_TEXTSECTION_WIDTH,
                TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT);

    // static space image screen
    textScrollerscreen1 = createScreen(spaceBlob, TRUE,
                                       0, TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT + 6,
                                       TEXTSCROLLER_VIEW_WIDTH,
                                       TEXTSCROLLER_VIEW_SPACESECTION_HEIGHT,
                                       TEXTSCROLLER_BLOB_SPACE_DEPTH, NULL);
    if (!textScrollerscreen1) {
        writeLog("Error: Could not allocate memory for space background screen\n");
        goto __exit_init_scroller;
    }
    LoadRGB32(&textScrollerscreen1->ViewPort, colortable1);

    // Make Screens visible on screen
    ScreenToFront(textScrollerScreen0);
    ScreenToFront(textScrollerscreen1);

    // init text scroller engine
    if (!initTextController(textscrollerScreen,
                            TEXTSCROLLER_BLOB_FONT_DEPTH,
                            TEXTSCROLLER_VIEW_TEXTSECTION_WIDTH)) {
        goto __exit_init_scroller;
    }
    return FSM_TEXTSCROLLER;

__exit_init_scroller:
    exitTextScroller();
    return FSM_ERROR;
}

void exitTextScroller(void) {
    writeLog("\n== exitTextScroller() ==\n");
    exitTextController();

    // restore old screen
    if (textScrollerScreen0) {
        CloseScreen(textScrollerScreen0);
        textScrollerScreen0 = NULL;
    }
    if (textScrollerscreen1) {
        CloseScreen(textScrollerscreen1);
        textScrollerscreen1 = NULL;
    }

    // restore screen elements
    if (colortable1) {
        FreeVec(colortable1);
        colortable1 = NULL;
    }

    if (textscrollerScreen) {
        FreeBitMap(textscrollerScreen);
        textscrollerScreen = NULL;
    }

    if (spaceBlob) {
        FreeBitMap(spaceBlob);
        spaceBlob = NULL;
    }

    payloadTextScrollerState = TEXTSCROLLER_INIT;
}

void fadeToWhite(void) {
    UWORD i = 0;
    UWORD incrementer;
    ULONG currentColor;

    // fade of text scroll area (viewPort[0])
    for (; i < TEXTSCROLLER_BLOB_FONT_COLORS; i++) {
        incrementer = 0;
        if ((colortable0[i] & 0x000f) != 0x000f) {
            incrementer |= 0x0001;
        }
        if ((colortable0[i] & 0x00f0) != 0x00f0) {
            incrementer |= 0x0010;
        }
        if ((colortable0[i] & 0x0f00) != 0x0f00) {
            incrementer |= 0x0100;
        }
        colortable0[i] += incrementer;
    }

    // fade of space background area (viewPort[1])
    for (i = 1; i < COLORMAP32_LONG_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS) - 1; i++) {
        currentColor = (colortable1[i] & 0x000000ff);
        if (currentColor == 0xff) {
            continue;
        }

        currentColor += 17;
        if (currentColor > 0x000000ff) {
            currentColor = 0x000000ff;
        }

        colortable1[i] = SPREAD(currentColor);
    }

    // calculated new color sets, now we can update copper and co
    WaitTOF();
    WaitBOVP(&textScrollerScreen0->ViewPort);
    LoadRGB4(&textScrollerScreen0->ViewPort, colortable0, TEXTSCROLLER_BLOB_FONT_COLORS);
    WaitBOVP(&textScrollerscreen1->ViewPort);
    LoadRGB32(&textScrollerscreen1->ViewPort, colortable1);
}

BOOL hasFadeToWhiteFinished(void) {
    UWORD i = 0;

    for (; i < TEXTSCROLLER_BLOB_FONT_COLORS; i++) {
        if ((colortable0[i] & 0x0fff) != 0x0fff) {
            return FALSE;
        }
    }

    for (i = 1; i < COLORMAP32_LONG_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS) - 1; i++) {
        if ((colortable1[i] & 0x000000ff) != 0xff) {
            return FALSE;
        }
    }
    return TRUE;
}
