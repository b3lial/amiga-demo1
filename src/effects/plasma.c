#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <graphics/scale.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/exec_protos.h>

#include "plasma.h"
#include "fsmstates.h"
#include "utils/utils.h"
#include "gfx/graphicscontroller.h"
#include "gfx/chunkyconverter.h"

struct PlasmaContext {
    enum PlasmaState state;
    struct BitMap *screenBitmaps[2];
    struct Screen *plasmaScreens[2];
    ULONG colorTable32[COLORMAP32_LONG_SIZE(PLASMA_SCREEN_COLORS)];
    UBYTE currentBufferIndex;
    UBYTE *chunkyBuffer;          // half resolution plasma, fast RAM
    struct BitMap *smallBitmap;   // half resolution planar copy, chip RAM (blitter source)
    UWORD frame;
};

static struct PlasmaContext ctx = {
    .state = PLASMA_INIT,
    .screenBitmaps = {NULL, NULL},
    .plasmaScreens = {NULL, NULL},
    .colorTable32 = {0},
    .currentBufferIndex = 0,
    .chunkyBuffer = NULL,
    .smallBitmap = NULL,
    .frame = 0
};

//----------------------------------------
// Fill the half resolution chunky buffer.
// TODO: replace this placeholder pattern with the real plasma
static void computePlasma(void) {
    UBYTE *pixel = ctx.chunkyBuffer;
    UWORD x;
    UWORD y;

    for (y = 0; y < PLASMA_CHUNKY_HEIGHT; y++) {
        for (x = 0; x < PLASMA_CHUNKY_WIDTH; x++) {
            *pixel++ = (UBYTE)(x + y + ctx.frame);
        }
    }
}

//----------------------------------------
// chunky (fast RAM) -> c2p -> small planar bitmap -> blitter 2x scale -> screen bitmap
static void renderFrame(struct BitMap *destination) {
    struct c2pStruct c2p;
    struct BitScaleArgs scale = {0};

    computePlasma();

    c2p.bmap = ctx.smallBitmap;
    c2p.startX = 0;
    c2p.startY = 0;
    c2p.width = PLASMA_CHUNKY_WIDTH;
    c2p.height = PLASMA_CHUNKY_HEIGHT;
    c2p.chunkybuffer = ctx.chunkyBuffer;
    ChunkyToPlanarAsm(&c2p);

    scale.bsa_SrcWidth = PLASMA_CHUNKY_WIDTH;
    scale.bsa_SrcHeight = PLASMA_CHUNKY_HEIGHT;
    scale.bsa_XSrcFactor = 1;
    scale.bsa_YSrcFactor = 1;
    scale.bsa_XDestFactor = 2;
    scale.bsa_YDestFactor = 2;
    scale.bsa_SrcBitMap = ctx.smallBitmap;
    scale.bsa_DestBitMap = destination;
    BitMapScale(&scale);
    WaitBlit();

    ctx.frame++;
}

//----------------------------------------
UWORD fsmPlasma(void) {
    UBYTE backBuffer;

    if (mouseClick()) {
        ctx.state = PLASMA_EXIT;
    }

    switch (ctx.state) {
        case PLASMA_INIT:
            ctx.state = PLASMA_ANIMATE;
            break;
        case PLASMA_ANIMATE:
            // render into the hidden screen, then show it at vertical blank
            backBuffer = 1 - ctx.currentBufferIndex;
            renderFrame(ctx.screenBitmaps[backBuffer]);
            WaitTOF();
            ScreenToFront(ctx.plasmaScreens[backBuffer]);
            ctx.currentBufferIndex = backBuffer;
            break;
        case PLASMA_EXIT:
            return FSM_PLASMA_FINISHED;
    }

    return FSM_PLASMA;
}

UWORD initPlasma(void) {
    UWORD i;
    UWORD pos;
    UBYTE sector;
    UBYTE rise;
    UBYTE fall;
    UBYTE r;
    UBYTE g;
    UBYTE b;
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

    // Half resolution chunky buffer in fast RAM (CPU renders here)
    ctx.chunkyBuffer = AllocVec(PLASMA_CHUNKY_WIDTH * PLASMA_CHUNKY_HEIGHT, MEMF_ANY | MEMF_CLEAR);
    if (!ctx.chunkyBuffer) {
        writeLog("Error: Could not allocate plasma chunky buffer\n");
        goto __exit_init_plasma;
    }

    // Half resolution planar bitmap in chip RAM, source for the blitter scaling
    ctx.smallBitmap = AllocBitMap(PLASMA_CHUNKY_WIDTH, PLASMA_CHUNKY_HEIGHT,
                                  PLASMA_SCREEN_DEPTH, BMF_CLEAR, NULL);
    if (!ctx.smallBitmap) {
        writeLog("Error: Could not allocate plasma small bitmap\n");
        goto __exit_init_plasma;
    }

    // Build cyclic rainbow palette in LoadRGB32 format (AGA 8-bit per channel)
    // Format: header=(count<<16)|start_reg, RGB triplets, 0 terminator
    ctx.colorTable32[0] = ((ULONG)PLASMA_SCREEN_COLORS << 16) | 0;

    // Walk the hue circle as 6 sectors of 256 steps each (full saturation/value).
    // Last entry wraps seamlessly back to entry 0, so the palette can be cycled.
    for (i = 0; i < PLASMA_SCREEN_COLORS; i++) {
        pos = (UWORD)((ULONG)i * 6 * 256 / PLASMA_SCREEN_COLORS);
        sector = (UBYTE)(pos >> 8);
        rise = (UBYTE)(pos & 0xFF);
        fall = (UBYTE)(255 - rise);

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

    // Render the first frame before showing the screen, otherwise the
    // cleared bitmap would flash in palette color 0 (red)
    renderFrame(ctx.screenBitmaps[ctx.currentBufferIndex]);

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

    if (ctx.smallBitmap) {
        FreeBitMap(ctx.smallBitmap);
        ctx.smallBitmap = NULL;
    }

    if (ctx.chunkyBuffer) {
        FreeVec(ctx.chunkyBuffer);
        ctx.chunkyBuffer = NULL;
    }

    // Reset state, buffer index and animation
    ctx.state = PLASMA_INIT;
    ctx.currentBufferIndex = 0;
    ctx.frame = 0;

    writeLog("Plasma effect cleaned up\n");
}
