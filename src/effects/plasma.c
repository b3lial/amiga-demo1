#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/exec_protos.h>

#include "plasma.h"
#include "fsmstates.h"
#include "utils/utils.h"
#include "gfx/graphicscontroller.h"

struct PlasmaContext {
    enum PlasmaState state;
    struct BitMap *screenBitmaps[2];
    struct Screen *plasmaScreens[2];
    ULONG colorTable32[COLORMAP32_LONG_SIZE(PLASMA_SCREEN_COLORS)];
    UBYTE currentBufferIndex;
};

static struct PlasmaContext ctx = {
    .state = PLASMA_INIT,
    .screenBitmaps = {NULL, NULL},
    .plasmaScreens = {NULL, NULL},
    .colorTable32 = {0},
    .currentBufferIndex = 0
};

UWORD fsmPlasma(void) {
    if (mouseClick()) {
        ctx.state = PLASMA_EXIT;
    }

    switch (ctx.state) {
        case PLASMA_INIT:
            ctx.state = PLASMA_ANIMATE;
            break;
        case PLASMA_ANIMATE:
            // TODO: implement plasma effect
            break;
        case PLASMA_EXIT:
            return FSM_PLASMA_FINISHED;
    }

    return FSM_PLASMA;
}

UWORD initPlasma(void) {
    writeLog("\n\n== initPlasma() ==\n");

    // Allocate first screen bitmap for double buffering
    ctx.screenBitmaps[0] = AllocBitMap(PLASMA_SCREEN_WIDTH, PLASMA_SCREEN_HEIGHT,
                                       PLASMA_SCREEN_DEPTH, BMF_DISPLAYABLE | BMF_CLEAR, NULL);
    if (!ctx.screenBitmaps[0]) {
        writeLog("Error: Could not allocate plasma bitmap 0\n");
        goto __exit_init_plasma;
    }

    // Allocate second screen bitmap for double buffering
    ctx.screenBitmaps[1] = AllocBitMap(PLASMA_SCREEN_WIDTH, PLASMA_SCREEN_HEIGHT,
                                       PLASMA_SCREEN_DEPTH, BMF_DISPLAYABLE | BMF_CLEAR, NULL);
    if (!ctx.screenBitmaps[1]) {
        writeLog("Error: Could not allocate plasma bitmap 1\n");
        goto __exit_init_plasma;
    }

    // Build cyclic rainbow palette in LoadRGB32 format (AGA 8-bit per channel)
    // Format: header=(count<<16)|start_reg, RGB triplets, 0 terminator
    ctx.colorTable32[0] = ((ULONG)PLASMA_SCREEN_COLORS << 16) | 0;

    // Walk the hue circle as 6 sectors of 256 steps each (full saturation/value).
    // Last entry wraps seamlessly back to entry 0, so the palette can be cycled.
    for (UWORD i = 0; i < PLASMA_SCREEN_COLORS; i++) {
        UWORD pos = (ULONG)i * 6 * 256 / PLASMA_SCREEN_COLORS;
        UBYTE sector = pos >> 8;
        UBYTE rise = pos & 0xFF;
        UBYTE fall = 255 - rise;
        UBYTE r, g, b;

        switch (sector) {
            case 0:  r = 255;  g = rise; b = 0;    break; // red -> yellow
            case 1:  r = fall; g = 255;  b = 0;    break; // yellow -> green
            case 2:  r = 0;    g = 255;  b = rise; break; // green -> cyan
            case 3:  r = 0;    g = fall; b = 255;  break; // cyan -> blue
            case 4:  r = rise; g = 0;    b = 255;  break; // blue -> magenta
            default: r = 255;  g = 0;    b = fall; break; // magenta -> red
        }

        ctx.colorTable32[1 + i * 3 + 0] = SPREAD(r);
        ctx.colorTable32[1 + i * 3 + 1] = SPREAD(g);
        ctx.colorTable32[1 + i * 3 + 2] = SPREAD(b);
    }
    ctx.colorTable32[COLORMAP32_LONG_SIZE(PLASMA_SCREEN_COLORS) - 1] = 0;

    // Create first screen (hidden initially)
    ctx.plasmaScreens[0] = createScreen(ctx.screenBitmaps[0], TRUE,
                                        0, 0, PLASMA_SCREEN_WIDTH, PLASMA_SCREEN_HEIGHT,
                                        PLASMA_SCREEN_DEPTH, NULL);
    if (!ctx.plasmaScreens[0]) {
        writeLog("Error: Could not create plasma screen 0\n");
        goto __exit_init_plasma;
    }

    // Create second screen for double buffering (hidden initially)
    ctx.plasmaScreens[1] = createScreen(ctx.screenBitmaps[1], TRUE,
                                        0, 0, PLASMA_SCREEN_WIDTH, PLASMA_SCREEN_HEIGHT,
                                        PLASMA_SCREEN_DEPTH, NULL);
    if (!ctx.plasmaScreens[1]) {
        writeLog("Error: Could not create plasma screen 1\n");
        goto __exit_init_plasma;
    }

    // Init double buffering - start with buffer 0
    ctx.currentBufferIndex = 0;

    // Load color tables for both screens using AGA LoadRGB32
    LoadRGB32(&ctx.plasmaScreens[0]->ViewPort, ctx.colorTable32);
    LoadRGB32(&ctx.plasmaScreens[1]->ViewPort, ctx.colorTable32);

    // Make screen visible
    ScreenToFront(ctx.plasmaScreens[ctx.currentBufferIndex]);

    writeLog("Plasma effect initialized with rainbow palette\n");
    return FSM_PLASMA;

__exit_init_plasma:
    exitPlasma();
    return FSM_ERROR;
}

void exitPlasma(void) {
    UBYTE i;
    writeLog("\n== exitPlasma() ==\n");

    // Wait for vertical blank before closing screens to avoid visual artifacts
    WaitTOF();
    for (i = 0; i < 2; i++) {
        if (ctx.plasmaScreens[i]) {
            CloseScreen(ctx.plasmaScreens[i]);
            ctx.plasmaScreens[i] = NULL;
        }
    }
    WaitTOF();

    // Free screen bitmaps
    for (i = 0; i < 2; i++) {
        if (ctx.screenBitmaps[i]) {
            FreeBitMap(ctx.screenBitmaps[i]);
            ctx.screenBitmaps[i] = NULL;
        }
    }

    // Reset state and buffer index
    ctx.state = PLASMA_INIT;
    ctx.currentBufferIndex = 0;

    writeLog("Plasma effect cleaned up\n");
}
