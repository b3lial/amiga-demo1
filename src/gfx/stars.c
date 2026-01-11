#include <exec/types.h>
#include <graphics/rastport.h>
#include <clib/graphics_protos.h>
#include <clib/alib_protos.h>

#include "stars.h"

#include "utils/utils.h"

struct StarsContext {
    UWORD xStars[STAR_MAX];
    UWORD yStars[STAR_MAX];
    UWORD xSpeed[STAR_MAX];  // movement pixels per tick (0, 1, or 2)
    UWORD width;
    UWORD height;
    UWORD borderX;
    UWORD borderY;
};

static struct StarsContext ctx = {
    .xStars = {0},
    .yStars = {0},
    .xSpeed = {0},
    .width = 0,
    .height = 0,
    .borderX = 0,
    .borderY = 0
};

//----------------------------------------
void createStars(UWORD numStars, UWORD width, UWORD height, UWORD borderX, UWORD borderY) {
    UWORD i;
    UWORD visibleWidth, visibleHeight;

    writeLogFS("initStars with numStars==%d\n", numStars);

    // Store dimensions in context
    ctx.width = width;
    ctx.height = height;
    ctx.borderX = borderX;
    ctx.borderY = borderY;

    // Calculate visible area (excluding borders)
    visibleWidth = width - (2 * borderX);
    visibleHeight = height - (2 * borderY);

    for (i = 0; i < numStars; i++) {
        // Create stars only in visible area (offset by border)
        ctx.xStars[i] = borderX + RangeRand(visibleWidth);
        ctx.yStars[i] = borderY + RangeRand(visibleHeight);
        // Random movement speed: 1, or 2 pixels per tick
        ctx.xSpeed[i] = 1 + RangeRand(2);
    }
}

//----------------------------------------
void paintStars(struct RastPort* rp, UWORD color, UWORD numStars) {
    UWORD i;
    ULONG currentColor;

    SetAPen(rp, color);
    for (i = 0; i < numStars; i++) {
        currentColor = ReadPixel(rp, ctx.xStars[i], ctx.yStars[i]);
        if (currentColor == 0) {
            WritePixel(rp, ctx.xStars[i], ctx.yStars[i]);
        }
    }
}

//----------------------------------------
void moveStars(UWORD numStars) {
    UWORD i;
    UWORD visibleWidth, visibleHeight;
    UWORD maxX;

    visibleWidth = ctx.width - (2 * ctx.borderX);
    visibleHeight = ctx.height - (2 * ctx.borderY);
    maxX = ctx.borderX + visibleWidth;

    for (i = 0; i < numStars; i++) {
        // Move star to the right by its speed
        ctx.xStars[i] += ctx.xSpeed[i];

        // Wrap around when star reaches right edge of visible area
        if (ctx.xStars[i] >= maxX) {
            ctx.xStars[i] = ctx.borderX;
            // Randomize y position within visible area when wrapping
            ctx.yStars[i] = ctx.borderY + RangeRand(visibleHeight);
        }
    }
}
