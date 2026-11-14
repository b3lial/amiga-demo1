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
    ULONG colorTable32[1 + PLASMA_SCREEN_COLORS * 3 + 1];
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

    // Build beautiful rainbow plasma palette using LoadRGB32 format (AGA 8-bit per channel)
    // Format: header=(count<<16)|start_reg, then RGB triplets as 0xRRRRRRRR/0xGGGGGGGG/0xBBBBBBBB
    ctx.colorTable32[0] = ((ULONG)PLASMA_SCREEN_COLORS << 16) | 0;

    // Generate smooth rainbow gradient through multiple hues
    for (UWORD i = 0; i < PLASMA_SCREEN_COLORS; i++) {
        ULONG hue = (i * 360) / PLASMA_SCREEN_COLORS;
        
        // HSV to RGB conversion (integer math, no libm)
        UBYTE sector = (hue / 60) % 6;
        ULONG fractionalHue = (hue * 256) / 60;
        UBYTE p = 0;
        UBYTE q = (256 * (256 - fractionalHue)) >> 8;
        UBYTE t = (256 * fractionalHue) >> 8;
        
        // Full saturation and value for vibrant colors
        UBYTE r, g, b;
        
        switch (sector) {
            case 0: // Red to yellow
                r = 255; g = t; b = p; break;
            case 1: // Yellow to green
                r = q; g = 255; b = p; break;
            case 2: // Green to cyan
                r = p; g = 255; b = t; break;
            case 3: // Cyan to blue
                r = p; g = q; b = 255; break;
            case 4: // Blue to magenta
                r = t; g = p; b = 255; break;
            case 5: // Magenta to red
                r = 255; g = p; b = q; break;
            default:
                r = 255; g = 0; b = 0; break;
        }
        
        // Replicate 8-bit value across all 4 bytes for LoadRGB32
        ctx.colorTable32[1 + i * 3 + 0] = ((ULONG)r << 24) | ((ULONG)r << 16) | ((ULONG)r << 8) | r;
        ctx.colorTable32[1 + i * 3 + 1] = ((ULONG)g << 24) | ((ULONG)g << 16) | ((ULONG)g << 8) | g;
        ctx.colorTable32[1 + i * 3 + 2] = ((ULONG)b << 24) | ((ULONG)b << 16) | ((ULONG)b << 8) | b;
    }

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
