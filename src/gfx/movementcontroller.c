#include "movementcontroller.h"

struct MovementControllerContext {
    UWORD centerX;
    UWORD centerY;
    UWORD logoHalfWidth;
    UWORD logoHalfHeight;
    UWORD currentIndex;
};

static struct MovementControllerContext ctx = {
    .centerX = 0,
    .centerY = 0,
    .logoHalfWidth = 0,
    .logoHalfHeight = 0,
    .currentIndex = 0
};

//----------------------------------------
void initMovementController(UWORD screenWidth, UWORD screenHeight, UWORD logoWidth, UWORD logoHeight) {
    // Calculate center of screen
    ctx.centerX = screenWidth / 2;
    ctx.centerY = screenHeight / 2;

    // Store half of logo dimensions for offset calculation
    ctx.logoHalfWidth = logoWidth / 2;
    ctx.logoHalfHeight = logoHeight / 2;

    // Reset index to start position
    ctx.currentIndex = 0;
}

//----------------------------------------
void getNextPosition(WORD *x, WORD *y) {
    // Get current circle coordinates (relative to 0,0)
    WORD circleX = circleCoords[ctx.currentIndex].x;
    WORD circleY = circleCoords[ctx.currentIndex].y;

    // Convert from fixed point and add to screen center
    // Subtract half logo dimensions so the logo center is at the circle position
    *x = ctx.centerX + FIXTOINT(circleX) - ctx.logoHalfWidth;
    *y = ctx.centerY + FIXTOINT(circleY) - ctx.logoHalfHeight;

    // Increment index and wrap around
    ctx.currentIndex = (ctx.currentIndex + 1) % CIRCLE_COORDS_COUNT;
}

