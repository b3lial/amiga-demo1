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
