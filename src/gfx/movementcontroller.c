#include "movementcontroller.h"

struct MovementControllerContext {
    UWORD centerX;
    UWORD centerY;
    UWORD currentIndex;
};

static struct MovementControllerContext ctx = {
    .centerX = 0,
    .centerY = 0,
    .currentIndex = 0
};

//----------------------------------------
void initMovementController(UWORD screenWidth, UWORD screenHeight) {
    // Calculate center of screen
    ctx.centerX = screenWidth / 2;
    ctx.centerY = screenHeight / 2;

    // Reset index to start position
    ctx.currentIndex = 0;
}

//----------------------------------------
void getNextPosition(WORD *x, WORD *y) {
    // Get current circle coordinates (relative to 0,0)
    WORD circleX = circleCoords[ctx.currentIndex].x;
    WORD circleY = circleCoords[ctx.currentIndex].y;

    // Convert from fixed point and add to screen center
    *x = ctx.centerX + FIXTOINT(circleX);
    *y = ctx.centerY + FIXTOINT(circleY);

    // Increment index and wrap around
    ctx.currentIndex = (ctx.currentIndex + 1) % CIRCLE_COORDS_COUNT;
}

