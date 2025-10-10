#include <exec/types.h>
#include <graphics/rastport.h>
#include <clib/graphics_protos.h>
#include <clib/alib_protos.h>

#include "stars.h"

#include "utils/utils.h"

struct StarsContext {
    UWORD xStars[STAR_MAX];
    UWORD yStars[STAR_MAX];
};

static struct StarsContext ctx = {
    .xStars = {0},
    .yStars = {0}
};

void createStars(UWORD numStars, UWORD width, UWORD height) {
    UWORD i;
    writeLogFS("initStars with numStars==%d\n", numStars);

    for (i = 0; i < numStars; i++) {
        ctx.xStars[i] = RangeRand(width);
        ctx.yStars[i] = RangeRand(height);
    }
}

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
