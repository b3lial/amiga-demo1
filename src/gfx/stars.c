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
};

static struct StarsContext ctx = {
    .xStars = {0},
    .yStars = {0},
    .xSpeed = {0},
    .width = 0,
    .height = 0
};

//----------------------------------------
void createStars(UWORD numStars, UWORD width, UWORD height) {
    UWORD i;
    writeLogFS("initStars with numStars==%d\n", numStars);

    // Store dimensions in context
    ctx.width = width;
    ctx.height = height;

    for (i = 0; i < numStars; i++) {
        ctx.xStars[i] = RangeRand(width);
        ctx.yStars[i] = RangeRand(height);
        // Random movement speed: 1, or 2 pixels per tick
        ctx.xSpeed[i] = 1 + RangeRand(2);
    }
}

//----------------------------------------
void paintStars(struct RastPort* rp, UWORD color, UWORD numStars, UWORD width, UWORD height) {
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

    for (i = 0; i < numStars; i++) {
        // Move star to the right by its speed
        ctx.xStars[i] += ctx.xSpeed[i];

        // Wrap around when star reaches right border
        if (ctx.xStars[i] >= ctx.width) {
            ctx.xStars[i] = 0;
            // Optionally randomize y position when wrapping
            ctx.yStars[i] = RangeRand(ctx.height);
        }
    }
}
