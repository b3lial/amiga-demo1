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

// LoadRGB32 table layout: 1 header ULONG + 3 ULONGs per color + 1 terminator
#define PALETTE_TABLE_SIZE (1 + MANDELBROT_SCREEN_COLORS * 3 + 1)

// Smooth iteration value per pixel (UWORD, 6 fractional bits).
// Encoding: sv = iter * 64 + (255 - log2_frac8(|z|²)) / 4
// This keeps max sv = (MANDELBROT_MAX_ITER-1)*64+63 = 31999 safely in UWORD.
// In-set sentinel: 0xFFFF (never reachable by escaped pixels).
#define MAND_INSET   ((UWORD)0xFFFF)
#define MAND_MAX_SV  ((UWORD)((MANDELBROT_MAX_ITER - 1) * 64 + 63))
#define HIST_BINS    512

struct MandelbrotContext {
    enum MandelbrotState state;
    struct BitMap *screenBitmaps[2];   // Chip RAM, displayable (double buffered)
    struct Screen *screens[2];
    struct BitMap *fastBitmap;         // Fast RAM bitmap for CPU rendering
    UWORD *iterBuf;                    // [SCREEN_HEIGHT * SCREEN_WIDTH] smooth values, Fast RAM
    ULONG colorTable32[PALETTE_TABLE_SIZE];  // LoadRGB32 format
    UBYTE currentBufferIndex;
};

static struct MandelbrotContext ctx = {
    .state = MANDELBROT_INIT,
    .screenBitmaps = {NULL, NULL},
    .screens = {NULL, NULL},
    .fastBitmap = NULL,
    .iterBuf = NULL,
    .colorTable32 = {0},
    .currentBufferIndex = 0
};

//----------------------------------------
// Oklab palette — integer only, no libm.
//
// Fixed-point scale: PFIX = 256 (8 fractional bits).
// Oklab L/a/b values scaled by 256.  Oklab L range 0..1 → 0..256.
// a/b range roughly -0.4..+0.4 → -102..+102 in this scale.
//
// sRGB gamma approximation via integer sqrt (no powf):
//   true gamma:  srgb = 1.055 * lin^(1/2.4) - 0.055
//   approx:      srgb ≈ sqrt(lin)  (error < 3%, invisible at 8-bit depth)
//   We use a bit-by-bit integer sqrt on the 0..65535 scale.
//----------------------------------------

#define PFIX 256

// Knot positions in units of (255 * PFIX) — i.e. scaled t * 255 * 256
static const UWORD knot_pos[12] = {
      0,  4608,  9984, 16640, 23296, 29952,
  36608, 43264, 49920, 55296, 60672, 65280
};

// Oklab knot colours, L/a/b each scaled by PFIX=256
// L: 0..256, a/b: signed, range ≈ -128..+128
typedef struct { WORD L, a, b; } OklabI;
static const OklabI knot_col[12] = {
    { 26,   0, -51},  // deep navy
    { 64,  -3, -77},  // blue
    {123, -18, -56},  // blue-cyan
    {179, -33, -18},  // cyan
    {223, -20,  -3},  // light cyan
    {243,   0,  13},  // near white
    {225,  -5,  56},  // yellow
    {171,  33,  49},  // orange
    {120,  51,  18},  // red
    { 79,  41,   8},  // dark red
    { 44,  23,   3},  // very dark red
    { 13,   3,   1},  // near-black
};

// Integer square root (bit-by-bit), input 0..65535, output 0..255
static UWORD isqrt16(UWORD n) {
    UWORD res = 0, bit = 0x4000;
    while (bit > 0) {
        UWORD tmp = res | bit;
        if ((ULONG)tmp * tmp <= (ULONG)n) res = tmp;
        bit >>= 1;
    }
    return res;
}

// Convert linear light [0..255] to sRGB [0..255] via sqrt approximation
static UBYTE linear_to_srgb8(WORD lin) {
    if (lin <= 0) return 0;
    if (lin >= 255) return 255;
    // sqrt(lin/255) * 255 = sqrt(lin*255)
    return (UBYTE)isqrt16((UWORD)((UWORD)lin * 255));
}

static void buildPalette(void) {
    UWORD i;
    UBYTE k;

    // Header: 256 colors starting at register 0
    ctx.colorTable32[0] = ((ULONG)MANDELBROT_SCREEN_COLORS << 16) | 0UL;

    // Index 0: black (in-set)
    ctx.colorTable32[1] = 0x00000000;
    ctx.colorTable32[2] = 0x00000000;
    ctx.colorTable32[3] = 0x00000000;

    // Indices 1..255: interpolate through knots in Oklab, then convert to sRGB
    for (i = 1; i < MANDELBROT_SCREEN_COLORS; i++) {
        // t in range 0..(255*PFIX), same scale as knot_pos
        ULONG t = (ULONG)i * (ULONG)(255 * PFIX) / (ULONG)(MANDELBROT_SCREEN_COLORS - 1);
        ULONG base = 1 + (ULONG)i * 3;
        WORD  iL, ia, ib;
        LONG  l_, m_, s_, l, m, s;
        WORD  rl, gl, bl;
        UBYTE r, g, b;
        WORD  alpha256;

        // Find surrounding knot pair
        k = 0;
        while (k < 10 && (ULONG)knot_pos[k + 1] < t) k++;

        {
            UWORD span = knot_pos[k+1] - knot_pos[k];
            UWORD dt   = (UWORD)(t - (ULONG)knot_pos[k]);
            // alpha in 0..256 (256 = 1.0)
            alpha256 = (span > 0) ? (WORD)((ULONG)dt * 256 / span) : 0;
        }

        // Interpolate L/a/b
        iL = knot_col[k].L + (WORD)(((LONG)(knot_col[k+1].L - knot_col[k].L) * alpha256) >> 8);
        ia = knot_col[k].a + (WORD)(((LONG)(knot_col[k+1].a - knot_col[k].a) * alpha256) >> 8);
        ib = knot_col[k].b + (WORD)(((LONG)(knot_col[k+1].b - knot_col[k].b) * alpha256) >> 8);

        // Oklab → linear RGB (all values scaled by PFIX=256)
        // Coefficients from the Oklab spec, scaled by 256:
        //   l_ = L + 0.3963*a + 0.2158*b  → *256: +101*a/256 + 55*b/256
        //   m_ = L - 0.1056*a - 0.0639*b  → *256: -27*a/256  - 16*b/256
        //   s_ = L - 0.0895*a - 1.2915*b  → *256: -23*a/256  - 331*b/256
        l_ = (LONG)iL + ((LONG)101 * ia + (LONG)55 * ib) / 256;
        m_ = (LONG)iL - ((LONG) 27 * ia + (LONG)16 * ib) / 256;
        s_ = (LONG)iL - ((LONG) 23 * ia + (LONG)331 * ib) / 256;

        // cube — values are in PFIX scale (0..256), cube overflows LONG if not clamped
        // clamp to 0..256 first
        if (l_ < 0) l_ = 0; else if (l_ > 256) l_ = 256;
        if (m_ < 0) m_ = 0; else if (m_ > 256) m_ = 256;
        if (s_ < 0) s_ = 0; else if (s_ > 256) s_ = 256;
        // cube: (l_/256)^3 * 256 = l_^3 / 256^2
        l = (l_ * l_ / 256) * l_ / 256;
        m = (m_ * m_ / 256) * m_ / 256;
        s = (s_ * s_ / 256) * s_ / 256;

        // Linear RGB from LMS (Oklab matrix, coefficients *256, then >>8 to keep PFIX scale)
        //  R =  4.0767*l - 3.3077*m + 0.2310*s
        //  G = -1.2684*l + 2.6098*m - 0.3413*s
        //  B = -0.0042*l - 0.7034*m + 1.7076*s
        rl = (WORD)(( 1044L * l - 847L * m +  59L * s) / 256);
        gl = (WORD)(( -325L * l + 668L * m -  87L * s) / 256);
        bl = (WORD)((   -1L * l - 180L * m + 437L * s) / 256);

        r = linear_to_srgb8(rl);
        g = linear_to_srgb8(gl);
        b = linear_to_srgb8(bl);

        ctx.colorTable32[base]     = (ULONG)r << 24;
        ctx.colorTable32[base + 1] = (ULONG)g << 24;
        ctx.colorTable32[base + 2] = (ULONG)b << 24;
    }

    // Terminator
    ctx.colorTable32[PALETTE_TABLE_SIZE - 1] = 0x00000000;
}

//----------------------------------------
// 8 fractional bits of log2(x), for x > 0.
// Returns the bits below the leading 1 of x, packed into a byte.
// Used for smooth coloring: gives fractional part of log2(|z|²) at escape.
static UBYTE log2_frac8(ULONG x) {
    UBYTE k = 0;
    ULONG t = x;
    if (t >= 0x10000UL) { t >>= 16; k += 16; }
    if (t >= 0x100UL)   { t >>= 8;  k += 8;  }
    if (t >= 0x10UL)    { t >>= 4;  k += 4;  }
    if (t >= 0x4UL)     { t >>= 2;  k += 2;  }
    if (t >= 0x2UL)     { t >>= 1;  k += 1;  }
    // k = floor(log2(x)); extract the 8 bits just below the leading 1
    if (k >= 8)
        return (UBYTE)(x >> (k - 8)) & 0xFF;
    else
        return (UBYTE)(x << (8 - k)) & 0xFF;
}

//----------------------------------------
// Fill ctx.iterBuf with smooth iteration values (UWORD per pixel, 6 frac bits).
// Encoding: sv = iter*64 + (63 - log2_frac8(|z|²)/4)
//   MAND_INSET (0xFFFF) → all 4 subsamples inside (true interior, black)
//   0..MAND_MAX_SV      → exterior
// Uses 2×2 supersampling. Coordinates in 14-bit fixed-point.
static void calculateMandelbrot(LONG x_start, LONG y_start, LONG x_end, LONG y_end) {
    UWORD mx, my;
    LONG dx = (x_end - x_start) / MANDELBROT_SCREEN_WIDTH;
    LONG dy = (y_end - y_start) / MANDELBROT_SCREEN_HEIGHT;
    LONG hdx = dx >> 1;
    LONG hdy = dy >> 1;

    for (my = 0; my < MANDELBROT_SCREEN_HEIGHT; my++) {
        LONG base_ci = y_start + (LONG)my * dy;
        for (mx = 0; mx < MANDELBROT_SCREEN_WIDTH; mx++) {
            LONG base_cr = x_start + (LONG)mx * dx;
            ULONG sum = 0;
            UBYTE inset_count = 0;
            UBYTE s;

            for (s = 0; s < 4; s++) {
                LONG c_r = base_cr + ((s & 1) ? hdx : 0);
                LONG c_i = base_ci + ((s & 2) ? hdy : 0);
                LONG z_r = 0, z_i = 0, z_r2 = 0, z_i2 = 0, new_zr;
                UWORD iter = 0;

                while (iter < MANDELBROT_MAX_ITER) {
                    z_r2 = MFIX_MUL(z_r, z_r);
                    z_i2 = MFIX_MUL(z_i, z_i);
                    if (z_r2 + z_i2 > 4L * MFIX_ONE) break;
                    new_zr = z_r2 - z_i2 + c_r;
                    z_i = 2L * MFIX_MUL(z_r, z_i) + c_i;
                    z_r = new_zr;
                    iter++;
                }

                if (iter >= MANDELBROT_MAX_ITER) {
                    inset_count++;
                    sum += (ULONG)MAND_MAX_SV;  // in-set subsample → max exterior value for avg
                } else {
                    UBYTE frac = log2_frac8((ULONG)(z_r2 + z_i2));
                    sum += (ULONG)iter * 64 + (ULONG)(63 - (frac >> 2));
                }
            }

            // Only mark as in-set if ALL 4 subsamples are inside
            if (inset_count == 4)
                ctx.iterBuf[(ULONG)my * MANDELBROT_SCREEN_WIDTH + mx] = MAND_INSET;
            else
                ctx.iterBuf[(ULONG)my * MANDELBROT_SCREEN_WIDTH + mx] = (UWORD)(sum >> 2);
        }
    }
}

//----------------------------------------
// Map ctx.iterBuf → ctx.fastBitmap planes via histogram equalization.
// 85% equalization blended with 15% log mapping for natural contrast.
// HIST_BINS buckets cover [0..MAND_MAX_SV], in-set pixels (MAND_INSET) → color 0.
static void renderToPlanes(void) {
    ULONG hist[HIST_BINS];
    UWORD mx, my;
    ULONG total_exterior = 0;
    ULONG cumsum, i;
    UWORD bytesPerRow = ctx.fastBitmap->BytesPerRow;

    // Build histogram over exterior pixels
    for (i = 0; i < HIST_BINS; i++) hist[i] = 0;
    for (i = 0; i < (ULONG)MANDELBROT_SCREEN_WIDTH * MANDELBROT_SCREEN_HEIGHT; i++) {
        UWORD sv = ctx.iterBuf[i];
        if (sv != MAND_INSET) {
            hist[(ULONG)sv * (HIST_BINS - 1) / MAND_MAX_SV]++;
            total_exterior++;
        }
    }

    // Build cumulative histogram → equalized palette index (1..255)
    // eq_map[bin] = 1 + cumulative_fraction * 254
    // Stored in hist[] in-place to save memory: overwrite with mapped value.
    cumsum = 0;
    for (i = 0; i < HIST_BINS; i++) {
        cumsum += hist[i];
        // 85% equalized + 15% linear (log-like at natural scale)
        if (total_exterior > 0) {
            ULONG eq_part  = cumsum * 254 / total_exterior;
            ULONG lin_part = i * 254 / (HIST_BINS - 1);
            hist[i] = (UBYTE)(1 + (eq_part * 85 + lin_part * 15) / 100);
        } else {
            hist[i] = 1;
        }
    }

    // Write planes
    for (my = 0; my < MANDELBROT_SCREEN_HEIGHT; my++) {
        for (mx = 0; mx < MANDELBROT_SCREEN_WIDTH; mx++) {
            UWORD sv = ctx.iterBuf[(ULONG)my * MANDELBROT_SCREEN_WIDTH + mx];
            UBYTE color, p;
            UWORD byteIdx = mx >> 3;
            UBYTE bitMask = 0x80 >> (mx & 7);

            if (sv == MAND_INSET) {
                color = 0;
            } else {
                ULONG bin = (ULONG)sv * (HIST_BINS - 1) / MAND_MAX_SV;
                color = (UBYTE)hist[bin];
            }

            for (p = 0; p < MANDELBROT_SCREEN_DEPTH; p++) {
                UBYTE *row = (UBYTE *)ctx.fastBitmap->Planes[p]
                           + (ULONG)my * bytesPerRow + byteIdx;
                if (color & (1 << p)) *row |= bitMask; else *row &= ~bitMask;
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

    // Allocate Fast RAM iteration buffer (one UWORD per pixel)
    ctx.iterBuf = (UWORD *)AllocVec(
        (ULONG)MANDELBROT_SCREEN_WIDTH * MANDELBROT_SCREEN_HEIGHT * sizeof(UWORD),
        MEMF_FAST | MEMF_CLEAR);
    if (!ctx.iterBuf) {
        writeLog("Error: Could not allocate iterBuf\n");
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

    // Build 256-color Oklab palette
    buildPalette();

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
    LoadRGB32(&ctx.screens[0]->ViewPort, ctx.colorTable32);
    LoadRGB32(&ctx.screens[1]->ViewPort, ctx.colorTable32);

    // Calculate Mandelbrot set into Fast RAM bitmap.
    // Triple Spiral Valley: centred on (-0.088, 0.654), half-width 0.1, half-height 0.075.
    writeLog("Calculating Mandelbrot...\n");
    calculateMandelbrot(
        FLOAT_TO_MFIX(-0.050), FLOAT_TO_MFIX( 0.635),
        FLOAT_TO_MFIX( 0.030), FLOAT_TO_MFIX( 0.695)
    );
    writeLog("Rendering to planes...\n");
    renderToPlanes();
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

    if (ctx.iterBuf) {
        FreeVec(ctx.iterBuf);
        ctx.iterBuf = NULL;
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
