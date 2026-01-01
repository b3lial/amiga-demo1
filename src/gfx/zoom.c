#include <exec/types.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>

#include "zoom.h"

#include "utils/utils.h"

#define MAX_ZOOM_STEPS 36

struct ZoomEngineContext {
    UBYTE *destBuffer[MAX_ZOOM_STEPS];
    UBYTE zoomSteps;
    UWORD bitmapWidth;
    UWORD bitmapHeight;
};

static struct ZoomEngineContext ctx = {
    .destBuffer = {NULL},
    .zoomSteps = 0,
    .bitmapWidth = 0,
    .bitmapHeight = 0
};

//----------------------------------------
BOOL startZoomEngine(UBYTE zoomSteps, USHORT bitmapWidth, USHORT bitmapHeight) {
    BYTE i = 0;

    ctx.zoomSteps = zoomSteps;
    ctx.bitmapWidth = bitmapWidth;
    ctx.bitmapHeight = bitmapHeight;

    if (ctx.zoomSteps == 0 || ctx.zoomSteps > MAX_ZOOM_STEPS) {
        writeLogFS("Error: Invalid zoom buffer size %d\n", ctx.zoomSteps);
        return FALSE;
    }

    // Allocate destination buffers for each zoom step
    for (i = 0; i < ctx.zoomSteps; i++) {
        ctx.destBuffer[i] = AllocVec(ctx.bitmapWidth * ctx.bitmapHeight, MEMF_FAST | MEMF_CLEAR);
        if (!(ctx.destBuffer[i])) {
            writeLog("Error: Could not allocate memory for zoom destination chunky buffer array\n");
            goto _error_cleanup;
        }
    }

    return TRUE;

_error_cleanup:
    exitZoomEngine();
    return FALSE;
}

//----------------------------------------
void exitZoomEngine(void) {
    BYTE i = 0;

    // Free all allocated destination buffers
    for (i = 0; i < MAX_ZOOM_STEPS; i++) {
        if (ctx.destBuffer[i]) {
            FreeVec(ctx.destBuffer[i]);
            ctx.destBuffer[i] = NULL;
        }
    }

    // Reset context
    ctx.zoomSteps = 0;
    ctx.bitmapWidth = 0;
    ctx.bitmapHeight = 0;
}

//----------------------------------------
void zoomBitmap(UBYTE *source, WORD fixZoomFactor, UBYTE index) {
    UWORD destX, destY;
    UWORD srcX, srcY;
    UWORD srcIndex, destIndex;
    UWORD destWidth, destHeight;
    UWORD destOffsetX, destOffsetY;
    UBYTE *dest;
    WORD fixInvZoomFactor;

    // Validate zoom factor (scale down only: 0 < fixZoomFactor <= 1.0 in fixed-point)
    if (fixZoomFactor <= 0 || fixZoomFactor > INTTOFIX(1)) {
        writeLog("Error: Invalid zoom factor (must be > 0.0 and <= 1.0)\n");
        return;
    }

    if (!source || index >= ctx.zoomSteps) {
        return;
    }

    dest = ctx.destBuffer[index];
    if (!dest) {
        return;
    }

    // Calculate destination dimensions (scaled down)
    destWidth = FIXTOINT(FIXMULT(INTTOFIX(ctx.bitmapWidth), fixZoomFactor));
    destHeight = FIXTOINT(FIXMULT(INTTOFIX(ctx.bitmapHeight), fixZoomFactor));

    // Calculate offset to center the zoomed image
    destOffsetX = (ctx.bitmapWidth - destWidth) / 2;
    destOffsetY = (ctx.bitmapHeight - destHeight) / 2;

    // Calculate inverse zoom factor for mapping dest -> src
    fixInvZoomFactor = safe_fixdiv(INTTOFIX(1), fixZoomFactor);

    // Scale down the bitmap using nearest neighbor sampling
    for (destY = 0; destY < destHeight; destY++) {
        for (destX = 0; destX < destWidth; destX++) {
            // Map destination pixel back to source pixel using fixed-point
            srcX = FIXTOINT(FIXMULT(INTTOFIX(destX), fixInvZoomFactor));
            srcY = FIXTOINT(FIXMULT(INTTOFIX(destY), fixInvZoomFactor));

            // Bounds check
            if (srcX < ctx.bitmapWidth && srcY < ctx.bitmapHeight) {
                srcIndex = srcX + srcY * ctx.bitmapWidth;
                destIndex = (destX + destOffsetX) + (destY + destOffsetY) * ctx.bitmapWidth;
                dest[destIndex] = source[srcIndex];
            }
        }
    }
}

//----------------------------------------
UBYTE* getZoomDestinationBuffer(UBYTE index) {
    if (index >= ctx.zoomSteps) {
        return NULL;
    }
    return ctx.destBuffer[index];
}
