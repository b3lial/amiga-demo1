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

#include "../gfx/text_controller.h"
#include "../fsm_states.h"
#include "../utils/utils.h"
#include "../gfx/stars.h"
#include "../gfx/graphics_controller.h"
#include "../gfx/blob_controller.h"

struct TextScrollerContext {
    WORD state;
    struct BitMap *spaceBlob;
    struct BitMap *textscrollerScreen;
    ULONG *colortable1;
    UWORD colortable0[TEXTSCROLLER_BLOB_FONT_COLORS];
    struct TextConfig *textList[TEXT_LIST_SIZE];
    struct TextConfig text1;
    struct TextConfig text2;
    struct TextConfig text3;
    struct Screen *textScrollerScreen0;
    struct Screen *textScrollerscreen1;
};

static struct TextScrollerContext ctx = {
    .state = TEXTSCROLLER_INIT,
    .spaceBlob = NULL,
    .textscrollerScreen = NULL,
    .colortable1 = NULL,
    .colortable0 = {0},
    .textList = {NULL},
    .text1 = {0},
    .text2 = {0},
    .text3 = {0},
    .textScrollerScreen0 = NULL,
    .textScrollerscreen1 = NULL
};

__far extern struct Custom custom;

UWORD fsmTextScroller(void) {
    // terminate effect on mouse click
    if (mouseClick()) {
        resetTextController();
        ctx.state = TEXTSCROLLER_FADE_WHITE;
    }

    // no mouse click -> execute state machine
    switch (ctx.state) {
        case TEXTSCROLLER_INIT:
            // configure text scroll engine
            ctx.text1.currentText = "hi there";
            ctx.text1.charXPosDestination = MAX_CHAR_WIDTH + 70;
            ctx.text1.charYPosDestination = 40;
            ctx.textList[0] = &ctx.text1;
            ctx.textList[1] = NULL;
            WaitTOF();
            setStringsTextController(ctx.textList);
            ctx.state = TEXTSCROLLER_MSG_1;
            break;

        // display "hi there"
        case TEXTSCROLLER_MSG_1:
            WaitTOF();
            executeTextController();
            if (isFinishedTextController()) {
                // configure text scroll engine
                resetTextController();
                ctx.text1.currentText = "belial";
                ctx.text1.charXPosDestination = MAX_CHAR_WIDTH + 95;
                ctx.text1.charYPosDestination = 18;
                ctx.text2.currentText = "here";
                ctx.text2.charXPosDestination = MAX_CHAR_WIDTH + 110;
                ctx.text2.charYPosDestination = 70;
                ctx.textList[0] = &ctx.text1;
                ctx.textList[1] = &ctx.text2;
                ctx.textList[2] = NULL;
                pauseTimeTextController(180);
                WaitTOF();
                setStringsTextController(ctx.textList);
                ctx.state = TEXTSCROLLER_MSG_2;
            }
            break;

        // display "belial here"
        case TEXTSCROLLER_MSG_2:
            WaitTOF();
            executeTextController();
            if (isFinishedTextController()) {
                // configure text scroll engine
                resetTextController();
                ctx.text1.currentText = "presenting";
                ctx.text1.charXPosDestination = MAX_CHAR_WIDTH + 37;
                ctx.text1.charYPosDestination = 4;
                ctx.text2.currentText = "my";
                ctx.text2.charXPosDestination = MAX_CHAR_WIDTH + 133;
                ctx.text2.charYPosDestination = 44;
                ctx.text3.currentText = "first";
                ctx.text3.charXPosDestination = MAX_CHAR_WIDTH + 103;
                ctx.text3.charYPosDestination = 84;
                ctx.textList[0] = &ctx.text1;
                ctx.textList[1] = &ctx.text2;
                ctx.textList[2] = &ctx.text3;
                ctx.textList[3] = NULL;
                pauseTimeTextController(660);
                WaitTOF();
                setStringsTextController(ctx.textList);
                ctx.state = TEXTSCROLLER_MSG_3;
            }
            break;

        // display "presenting my first"
        case TEXTSCROLLER_MSG_3:
            WaitTOF();
            executeTextController();
            if (isFinishedTextController()) {
                // configure text scroll engine
                resetTextController();
                ctx.text1.currentText = "demo";
                ctx.text1.charXPosDestination = MAX_CHAR_WIDTH + 10;
                ctx.text1.charYPosDestination = 18;
                ctx.text2.currentText = "production";
                ctx.text2.charXPosDestination = MAX_CHAR_WIDTH + 45;
                ctx.text2.charYPosDestination = 70;
                ctx.textList[0] = &ctx.text1;
                ctx.textList[1] = &ctx.text2;
                ctx.textList[2] = NULL;
                pauseTimeTextController(300);
                WaitTOF();
                setStringsTextController(ctx.textList);
                ctx.state = TEXTSCROLLER_MSG_4;
            }
            break;

        // display "demo production"
        case TEXTSCROLLER_MSG_4:
            WaitTOF();
            executeTextController();
            if (isFinishedTextController()) {
                ctx.state = TEXTSCROLLER_FADE_WHITE;
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
        ctx.textList[i] = NULL;
    }

    writeLog("Load space background bitmap and colors\n");
    // Load space background bitmap
    ctx.spaceBlob = loadBlob("img/space4_320_125_8.RAW", TEXTSCROLLER_BLOB_SPACE_DEPTH,
                         TEXTSCROLLER_BLOB_SPACE_WIDTH, TEXTSCROLLER_BLOB_SPACE_HEIGHT);
    if (ctx.spaceBlob == NULL) {
        writeLog("Error: Could not load space blob\n");
        goto __exit_init_scroller;
    }
    writeLogFS(
        "Space Bitmap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
        ctx.spaceBlob->BytesPerRow, ctx.spaceBlob->Rows, ctx.spaceBlob->Flags,
        ctx.spaceBlob->pad);

    // Load space background color table
    ctx.colortable1 = AllocVec(COLORMAP32_BYTE_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS), MEMF_ANY);
    if (!ctx.colortable1) {
        writeLog("Error: Could not allocate memory for space bitmap color table\n");
        goto __exit_init_scroller;
    }
    writeLogFS("Allocated %d  bytes for space bitmap color table\n",
               COLORMAP32_BYTE_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS));
    loadColorMap32("img/space3_320_148_8.CMAP", ctx.colortable1, TEXTSCROLLER_BLOB_SPACE_COLORS);

    // Load Textscroller Screen Bitmap
    ctx.textscrollerScreen = AllocBitMap(TEXTSCROLLER_VIEW_TEXTSECTION_WIDTH,
                                     TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT, TEXTSCROLLER_BLOB_FONT_DEPTH,
                                     BMF_CLEAR | BMF_DISPLAYABLE, NULL);
    writeLogFS("TextScroller Screen BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
               ctx.textscrollerScreen->BytesPerRow, ctx.textscrollerScreen->Rows,
               ctx.textscrollerScreen->Flags, ctx.textscrollerScreen->pad);

    // Load Textscroller color table
    loadColorMap("img/charset_final.CMAP", ctx.colortable0,
                 TEXTSCROLLER_BLOB_FONT_COLORS);

    writeLog("Create two screens\n");
    // text scroller section screen
    starsClip.MinX = 0;
    starsClip.MinY = 0;
    starsClip.MaxX = TEXTSCROLLER_VIEW_WIDTH;
    starsClip.MaxY = TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT;
    ctx.textScrollerScreen0 = createScreen(ctx.textscrollerScreen, TRUE,
                                       -MAX_CHAR_WIDTH, 0,
                                       TEXTSCROLLER_VIEW_TEXTSECTION_WIDTH,
                                       TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT,
                                       TEXTSCROLLER_BLOB_FONT_DEPTH, &starsClip);
    if (!ctx.textScrollerScreen0) {
        writeLog("Error: Could not allocate memory for text scroller screen\n");
        goto __exit_init_scroller;
    }
    LoadRGB4(&ctx.textScrollerScreen0->ViewPort, ctx.colortable0, TEXTSCROLLER_BLOB_FONT_COLORS);

    initStars(50, TEXTSCROLLER_VIEW_TEXTSECTION_WIDTH, TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT);
    createStars(&ctx.textScrollerScreen0->RastPort, 6, 50, TEXTSCROLLER_VIEW_TEXTSECTION_WIDTH,
                TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT);

    // static space image screen
    ctx.textScrollerscreen1 = createScreen(ctx.spaceBlob, TRUE,
                                       0, TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT + 6,
                                       TEXTSCROLLER_VIEW_WIDTH,
                                       TEXTSCROLLER_VIEW_SPACESECTION_HEIGHT,
                                       TEXTSCROLLER_BLOB_SPACE_DEPTH, NULL);
    if (!ctx.textScrollerscreen1) {
        writeLog("Error: Could not allocate memory for space background screen\n");
        goto __exit_init_scroller;
    }
    LoadRGB32(&ctx.textScrollerscreen1->ViewPort, ctx.colortable1);

    // Make Screens visible on screen
    ScreenToFront(ctx.textScrollerScreen0);
    ScreenToFront(ctx.textScrollerscreen1);

    // init text scroller engine
    if (!initTextController(ctx.textscrollerScreen,
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
    if (ctx.textScrollerScreen0) {
        CloseScreen(ctx.textScrollerScreen0);
        ctx.textScrollerScreen0 = NULL;
    }
    if (ctx.textScrollerscreen1) {
        CloseScreen(ctx.textScrollerscreen1);
        ctx.textScrollerscreen1 = NULL;
    }

    // restore screen elements
    if (ctx.colortable1) {
        FreeVec(ctx.colortable1);
        ctx.colortable1 = NULL;
    }

    if (ctx.textscrollerScreen) {
        FreeBitMap(ctx.textscrollerScreen);
        ctx.textscrollerScreen = NULL;
    }

    if (ctx.spaceBlob) {
        FreeBitMap(ctx.spaceBlob);
        ctx.spaceBlob = NULL;
    }

    ctx.state = TEXTSCROLLER_INIT;
}

void fadeToWhite(void) {
    UWORD i = 0;
    UWORD incrementer;
    ULONG currentColor;

    // fade of text scroll area (viewPort[0])
    for (; i < TEXTSCROLLER_BLOB_FONT_COLORS; i++) {
        incrementer = 0;
        if ((ctx.colortable0[i] & 0x000f) != 0x000f) {
            incrementer |= 0x0001;
        }
        if ((ctx.colortable0[i] & 0x00f0) != 0x00f0) {
            incrementer |= 0x0010;
        }
        if ((ctx.colortable0[i] & 0x0f00) != 0x0f00) {
            incrementer |= 0x0100;
        }
        ctx.colortable0[i] += incrementer;
    }

    // fade of space background area (viewPort[1])
    for (i = 1; i < COLORMAP32_LONG_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS) - 1; i++) {
        currentColor = (ctx.colortable1[i] & 0x000000ff);
        if (currentColor == 0xff) {
            continue;
        }

        currentColor += 17;
        if (currentColor > 0x000000ff) {
            currentColor = 0x000000ff;
        }

        ctx.colortable1[i] = SPREAD(currentColor);
    }

    // calculated new color sets, now we can update copper and co
    WaitTOF();
    WaitBOVP(&ctx.textScrollerScreen0->ViewPort);
    LoadRGB4(&ctx.textScrollerScreen0->ViewPort, ctx.colortable0, TEXTSCROLLER_BLOB_FONT_COLORS);
    WaitBOVP(&ctx.textScrollerscreen1->ViewPort);
    LoadRGB32(&ctx.textScrollerscreen1->ViewPort, ctx.colortable1);
}

BOOL hasFadeToWhiteFinished(void) {
    UWORD i = 0;

    for (; i < TEXTSCROLLER_BLOB_FONT_COLORS; i++) {
        if ((ctx.colortable0[i] & 0x0fff) != 0x0fff) {
            return FALSE;
        }
    }

    for (i = 1; i < COLORMAP32_LONG_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS) - 1; i++) {
        if ((ctx.colortable1[i] & 0x000000ff) != 0xff) {
            return FALSE;
        }
    }
    return TRUE;
}
