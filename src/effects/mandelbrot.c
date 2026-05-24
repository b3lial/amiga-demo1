#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <intuition/screens.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/exec_protos.h>

#include "mandelbrot.h"
#include "fsmstates.h"
#include "utils/utils.h"
#include "gfx/graphicscontroller.h"

struct MandelbrotContext {
    enum MandelbrotState state;
    struct BitMap *screenBitmaps[2];
    struct Screen *screens[2];
    UWORD colorTable[MANDELBROT_SCREEN_COLORS];  // LoadRGB4 format: 0x0RGB per entry
    UBYTE currentBufferIndex;  // 0 or 1
};

static struct MandelbrotContext ctx = {
    .state = MANDELBROT_INIT,
    .screenBitmaps = {NULL, NULL},
    .screens = {NULL, NULL},
    .colorTable = {0},
    .currentBufferIndex = 0
};

//----------------------------------------
UWORD initMandelbrot(void) {
    UBYTE i;
    writeLog("\n\n== initMandelbrot() ==\n");

    // Allocate first screen bitmap (chip RAM, displayable)
    ctx.screenBitmaps[0] = AllocBitMap(MANDELBROT_SCREEN_WIDTH, MANDELBROT_SCREEN_HEIGHT,
                                       MANDELBROT_SCREEN_DEPTH,
                                       BMF_DISPLAYABLE | BMF_CLEAR, NULL);
    if (!ctx.screenBitmaps[0]) {
        writeLog("Error: Could not allocate screen bitmap 0\n");
        goto __exit_init_mandelbrot;
    }

    // Allocate second screen bitmap for double buffering
    ctx.screenBitmaps[1] = AllocBitMap(MANDELBROT_SCREEN_WIDTH, MANDELBROT_SCREEN_HEIGHT,
                                       MANDELBROT_SCREEN_DEPTH,
                                       BMF_DISPLAYABLE | BMF_CLEAR, NULL);
    if (!ctx.screenBitmaps[1]) {
        writeLog("Error: Could not allocate screen bitmap 1\n");
        goto __exit_init_mandelbrot;
    }

    // Initialize color table (LoadRGB4 format: 0x0RGB per entry, 4 bits per channel)
    // Index 0: black (Mandelbrot interior / in-set pixels)
    // Indices 1..15: gradient from dark blue to bright cyan (exterior escape bands)
    ctx.colorTable[0] = 0x0000;  // black
    for (i = 1; i < MANDELBROT_SCREEN_COLORS; i++) {
        UWORD t = ((UWORD)(i - 1) * 0xF) / (MANDELBROT_SCREEN_COLORS - 2);
        UWORD r = t / 4;   // low red
        UWORD g = t / 2;   // medium green
        UWORD b = t;       // full blue
        ctx.colorTable[i] = ((r & 0xF) << 8) | ((g & 0xF) << 4) | (b & 0xF);
    }

    // Create first screen
    ctx.screens[0] = createScreen(ctx.screenBitmaps[0], TRUE,
                                  0, 0,
                                  MANDELBROT_SCREEN_WIDTH, MANDELBROT_SCREEN_HEIGHT,
                                  MANDELBROT_SCREEN_DEPTH, NULL);
    if (!ctx.screens[0]) {
        writeLog("Error: Could not create screen 0\n");
        goto __exit_init_mandelbrot;
    }

    // Create second screen for double buffering
    ctx.screens[1] = createScreen(ctx.screenBitmaps[1], TRUE,
                                  0, 0,
                                  MANDELBROT_SCREEN_WIDTH, MANDELBROT_SCREEN_HEIGHT,
                                  MANDELBROT_SCREEN_DEPTH, NULL);
    if (!ctx.screens[1]) {
        writeLog("Error: Could not create screen 1\n");
        goto __exit_init_mandelbrot;
    }

    ctx.currentBufferIndex = 0;
    LoadRGB4(&ctx.screens[0]->ViewPort, ctx.colorTable, MANDELBROT_SCREEN_COLORS);
    LoadRGB4(&ctx.screens[1]->ViewPort, ctx.colorTable, MANDELBROT_SCREEN_COLORS);

    ScreenToFront(ctx.screens[ctx.currentBufferIndex]);
    return FSM_MANDELBROT;

__exit_init_mandelbrot:
    exitMandelbrot();
    return FSM_ERROR;
}

//----------------------------------------
UWORD fsmMandelbrot(void) {
    if (mouseClick()) {
        ctx.state = MANDELBROT_EXIT;
    }

    switch (ctx.state) {
        case MANDELBROT_INIT:
            ctx.state = MANDELBROT_SHOW;
            break;
        case MANDELBROT_SHOW:
            break;
        case MANDELBROT_EXIT:
            return FSM_MANDELBROT_FINISHED;
    }

    return FSM_MANDELBROT;
}

//----------------------------------------
void exitMandelbrot(void) {
    UBYTE i;
    writeLog("\n== exitMandelbrot() ==\n");

    WaitTOF();
    for (i = 0; i < 2; i++) {
        if (ctx.screens[i]) {
            CloseScreen(ctx.screens[i]);
            ctx.screens[i] = NULL;
        }
    }
    WaitTOF();

    for (i = 0; i < 2; i++) {
        if (ctx.screenBitmaps[i]) {
            FreeBitMap(ctx.screenBitmaps[i]);
            ctx.screenBitmaps[i] = NULL;
        }
    }

    ctx.state = MANDELBROT_INIT;
    ctx.currentBufferIndex = 0;
}
