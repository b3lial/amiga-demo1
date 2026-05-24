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

// Fixed-point arithmetic for Mandelbrot iteration (14 fractional bits).
// 14 bits chosen so z_r*z_r fits in a signed LONG without 64-bit multiply:
// z is bounded by 2.0 before each multiply (escape check |z|²>4 keeps this safe),
// so z_r <= 2*16384 = 32768 and z_r*z_r <= 2^30 < LONG_MAX.
#define MFIX_SHIFT       14
#define MFIX_ONE         (1L << MFIX_SHIFT)
#define FLOAT_TO_MFIX(x) ((LONG)((x) * MFIX_ONE))
#define MFIX_MUL(a, b)   ((LONG)((LONG)(a) * (LONG)(b)) >> MFIX_SHIFT)

struct MandelbrotContext {
    enum MandelbrotState state;
    struct BitMap *screenBitmaps[2];   // Chip RAM, displayable (double buffered)
    struct Screen *screens[2];
    struct BitMap *fastBitmap;         // Fast RAM bitmap for CPU rendering
    UWORD colorTable[MANDELBROT_SCREEN_COLORS];  // LoadRGB4 format: 0x0RGB per entry
    UBYTE currentBufferIndex;          // 0 or 1
};

static struct MandelbrotContext ctx = {
    .state = MANDELBROT_INIT,
    .screenBitmaps = {NULL, NULL},
    .screens = {NULL, NULL},
    .fastBitmap = NULL,
    .colorTable = {0},
    .currentBufferIndex = 0
};

//----------------------------------------
// Render the Mandelbrot set into dest (a Fast RAM bitmap).
// Coordinates are in 14-bit fixed-point (MFIX_SHIFT).
// Each Mandelbrot pixel is written as a 2x2 block into the 320x256 bitplane layout.
// Pure CPU writes only — no blitter, no graphics.library (dest is in Fast RAM).
static void calculateMandelbrot(LONG x_start, LONG y_start, LONG x_end, LONG y_end,
                                UBYTE maxIterations, struct BitMap *dest) {
    UWORD mx, my;
    UWORD bytesPerRow = dest->BytesPerRow;
    LONG dx = (x_end - x_start) / MANDELBROT_WIDTH;
    LONG dy = (y_end - y_start) / MANDELBROT_HEIGHT;

    for (my = 0; my < MANDELBROT_HEIGHT; my++) {
        LONG c_i = y_start + (LONG)my * dy;
        UWORD sy = my * 2;  // screen Y of the 2x2 block top row

        for (mx = 0; mx < MANDELBROT_WIDTH; mx++) {
            LONG c_r = x_start + (LONG)mx * dx;
            LONG z_r = 0, z_i = 0;
            UBYTE iter = 0;

            // Iterate z = z² + c.
            // Escape is checked before the multiply so z is always bounded
            // by 2.0 at the time of multiplication — keeping products in 32 bits.
            while (iter < maxIterations) {
                LONG z_r2 = MFIX_MUL(z_r, z_r);
                LONG z_i2 = MFIX_MUL(z_i, z_i);
                if (z_r2 + z_i2 > 4L * MFIX_ONE) break;  // escaped
                LONG new_zr = z_r2 - z_i2 + c_r;
                z_i = 2L * MFIX_MUL(z_r, z_i) + c_i;
                z_r = new_zr;
                iter++;
            }

            // Color: 0 = in-set (never escaped), 1..maxIterations = escaped after N steps
            UBYTE color = (iter >= maxIterations) ? 0 : (UBYTE)(iter + 1);

            // Write 2x2 block into bitplanes via CPU.
            // sx is always even (mx*2), so both pixels always fall in the same byte.
            UWORD sx = mx * 2;
            UWORD byteIdx = sx >> 3;           // byte within the row
            UBYTE bitMask = 0xC0 >> (sx & 7);  // two adjacent bits

            UBYTE p;
            for (p = 0; p < MANDELBROT_SCREEN_DEPTH; p++) {
                UBYTE *plane = (UBYTE *)dest->Planes[p];
                UBYTE *row0 = plane + (ULONG)sy       * bytesPerRow + byteIdx;
                UBYTE *row1 = plane + (ULONG)(sy + 1) * bytesPerRow + byteIdx;
                if (color & (1 << p)) {
                    *row0 |= bitMask;
                    *row1 |= bitMask;
                } else {
                    *row0 &= ~bitMask;
                    *row1 &= ~bitMask;
                }
            }
        }
    }
}

//----------------------------------------
UWORD initMandelbrot(void) {
    UBYTE i;
    ULONG planeSize;
    writeLog("\n\n== initMandelbrot() ==\n");

    // Allocate screen bitmaps in Chip RAM for display
    ctx.screenBitmaps[0] = AllocBitMap(MANDELBROT_SCREEN_WIDTH, MANDELBROT_SCREEN_HEIGHT,
                                       MANDELBROT_SCREEN_DEPTH,
                                       BMF_DISPLAYABLE | BMF_CLEAR, NULL);
    if (!ctx.screenBitmaps[0]) {
        writeLog("Error: Could not allocate screen bitmap 0\n");
        goto __exit_init_mandelbrot;
    }

    ctx.screenBitmaps[1] = AllocBitMap(MANDELBROT_SCREEN_WIDTH, MANDELBROT_SCREEN_HEIGHT,
                                       MANDELBROT_SCREEN_DEPTH,
                                       BMF_DISPLAYABLE | BMF_CLEAR, NULL);
    if (!ctx.screenBitmaps[1]) {
        writeLog("Error: Could not allocate screen bitmap 1\n");
        goto __exit_init_mandelbrot;
    }

    // Allocate Fast RAM bitmap for CPU rendering.
    // Planes are explicitly placed in Fast RAM so the blitter never touches them.
    ctx.fastBitmap = (struct BitMap *)AllocVec(sizeof(struct BitMap), MEMF_ANY | MEMF_CLEAR);
    if (!ctx.fastBitmap) {
        writeLog("Error: Could not allocate fast bitmap struct\n");
        goto __exit_init_mandelbrot;
    }
    InitBitMap(ctx.fastBitmap, MANDELBROT_SCREEN_DEPTH,
               MANDELBROT_SCREEN_WIDTH, MANDELBROT_SCREEN_HEIGHT);
    planeSize = (ULONG)ctx.fastBitmap->BytesPerRow * MANDELBROT_SCREEN_HEIGHT;
    for (i = 0; i < MANDELBROT_SCREEN_DEPTH; i++) {
        ctx.fastBitmap->Planes[i] = AllocVec(planeSize, MEMF_FAST | MEMF_CLEAR);
        if (!ctx.fastBitmap->Planes[i]) {
            writeLog("Error: Could not allocate fast bitmap plane\n");
            goto __exit_init_mandelbrot;
        }
    }

    // Initialize color table (LoadRGB4 format: 0x0RGB per entry, 4 bits per channel)
    // Index 0: black (in-set pixels)
    // Indices 1..15: gradient from dark blue to bright cyan
    ctx.colorTable[0] = 0x0000;
    for (i = 1; i < MANDELBROT_SCREEN_COLORS; i++) {
        UWORD t = ((UWORD)(i - 1) * 0xF) / (MANDELBROT_SCREEN_COLORS - 2);
        UWORD r = t / 4;
        UWORD g = t / 2;
        UWORD b = t;
        ctx.colorTable[i] = ((r & 0xF) << 8) | ((g & 0xF) << 4) | (b & 0xF);
    }

    // Create screens
    ctx.screens[0] = createScreen(ctx.screenBitmaps[0], TRUE,
                                  0, 0,
                                  MANDELBROT_SCREEN_WIDTH, MANDELBROT_SCREEN_HEIGHT,
                                  MANDELBROT_SCREEN_DEPTH, NULL);
    if (!ctx.screens[0]) {
        writeLog("Error: Could not create screen 0\n");
        goto __exit_init_mandelbrot;
    }

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

    // Calculate Mandelbrot set into Fast RAM bitmap.
    // Default view: full set x=[-2.5, 1.0], y=[-1.25, 1.25], 15 iterations = 15 exterior colors.
    writeLog("Calculating Mandelbrot...\n");
    calculateMandelbrot(
        FLOAT_TO_MFIX(-2.5), FLOAT_TO_MFIX(-1.25),
        FLOAT_TO_MFIX(1.0),  FLOAT_TO_MFIX(1.25),
        MANDELBROT_SCREEN_COLORS - 1,
        ctx.fastBitmap
    );
    writeLog("Mandelbrot done.\n");

    ScreenToFront(ctx.screens[ctx.currentBufferIndex]);
    return FSM_MANDELBROT;

__exit_init_mandelbrot:
    exitMandelbrot();
    return FSM_ERROR;
}

//----------------------------------------
static void draw(void) {
    UBYTE i;
    ULONG planeSize = (ULONG)ctx.fastBitmap->BytesPerRow * MANDELBROT_SCREEN_HEIGHT;

    // Switch to back buffer
    ctx.currentBufferIndex = 1 - ctx.currentBufferIndex;

    // Copy Fast RAM bitmap into the back Chip RAM screen bitmap
    for (i = 0; i < MANDELBROT_SCREEN_DEPTH; i++) {
        CopyMem(ctx.fastBitmap->Planes[i],
                ctx.screenBitmaps[ctx.currentBufferIndex]->Planes[i],
                planeSize);
    }

    WaitTOF();
    ScreenToFront(ctx.screens[ctx.currentBufferIndex]);
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
            draw();
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

    if (ctx.fastBitmap) {
        for (i = 0; i < MANDELBROT_SCREEN_DEPTH; i++) {
            if (ctx.fastBitmap->Planes[i]) {
                FreeVec(ctx.fastBitmap->Planes[i]);
                ctx.fastBitmap->Planes[i] = NULL;
            }
        }
        FreeVec(ctx.fastBitmap);
        ctx.fastBitmap = NULL;
    }

    ctx.state = MANDELBROT_INIT;
    ctx.currentBufferIndex = 0;
}
