#include <exec/types.h>
#include <graphics/gfx.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>

#include "rotatingcube.h"

#include "fsmstates.h"
#include "utils/utils.h"
#include "gfx/graphicscontroller.h"

struct RotatingCubeContext {
    enum RotatingCubeState state;
    struct BitMap *screenBitmaps[2];
    struct Screen *cubeScreens[2];
    UWORD colorTable[ROTATINGCUBE_SCREEN_COLORS];
    UBYTE currentBufferIndex;  // 0 or 1
};

static struct RotatingCubeContext ctx = {
    .state = ROTATINGCUBE_INIT,
    .screenBitmaps = {NULL, NULL},
    .cubeScreens = {NULL, NULL},
    .colorTable = {0},
    .currentBufferIndex = 0
};

//----------------------------------------
UWORD fsmRotatingCube(void) {
    if (mouseClick()) {
        ctx.state = ROTATINGCUBE_SHUTDOWN;
    }

    switch (ctx.state) {
        case ROTATINGCUBE_INIT:
            ctx.state = ROTATINGCUBE_RUNNING;
            break;
        case ROTATINGCUBE_RUNNING:
            // TODO: Implement cube rotation logic
            WaitTOF();
            // Switch buffers
            ctx.currentBufferIndex = 1 - ctx.currentBufferIndex;
            ScreenToFront(ctx.cubeScreens[ctx.currentBufferIndex]);
            break;
        case ROTATINGCUBE_SHUTDOWN:
            exitRotatingCube();
            return FSM_STOP;
    }

    return FSM_ROTATINGCUBE;
}

//----------------------------------------
UWORD initRotatingCube(void) {
    UBYTE i;
    writeLog("\n\n== initRotatingCube() ==\n");

    // Allocate first screen bitmap
    ctx.screenBitmaps[0] = AllocBitMap(ROTATINGCUBE_SCREEN_WIDTH,
                                       ROTATINGCUBE_SCREEN_HEIGHT,
                                       ROTATINGCUBE_SCREEN_DEPTH,
                                       BMF_DISPLAYABLE | BMF_CLEAR,
                                       NULL);
    if (!ctx.screenBitmaps[0]) {
        writeLog("Error: Could not allocate memory for rotating cube bitmap 0\n");
        goto __exit_init_cube;
    }

    // Allocate second screen bitmap for double buffering
    ctx.screenBitmaps[1] = AllocBitMap(ROTATINGCUBE_SCREEN_WIDTH,
                                       ROTATINGCUBE_SCREEN_HEIGHT,
                                       ROTATINGCUBE_SCREEN_DEPTH,
                                       BMF_DISPLAYABLE | BMF_CLEAR,
                                       NULL);
    if (!ctx.screenBitmaps[1]) {
        writeLog("Error: Could not allocate memory for rotating cube bitmap 1\n");
        goto __exit_init_cube;
    }

    // Initialize color table (simple grayscale for now)
    for (i = 0; i < ROTATINGCUBE_SCREEN_COLORS; i++) {
        UWORD intensity = (i * 0xf) / (ROTATINGCUBE_SCREEN_COLORS - 1);
        ctx.colorTable[i] = (intensity << 8) | (intensity << 4) | intensity;
    }

    // Create first screen
    ctx.cubeScreens[0] = createScreen(ctx.screenBitmaps[0], TRUE,
                                      0, 0,
                                      ROTATINGCUBE_SCREEN_WIDTH,
                                      ROTATINGCUBE_SCREEN_HEIGHT,
                                      ROTATINGCUBE_SCREEN_DEPTH, NULL);
    if (!ctx.cubeScreens[0]) {
        writeLog("Error: Could not allocate memory for rotating cube screen 0\n");
        goto __exit_init_cube;
    }

    // Create second screen for double buffering
    ctx.cubeScreens[1] = createScreen(ctx.screenBitmaps[1], TRUE,
                                      0, 0,
                                      ROTATINGCUBE_SCREEN_WIDTH,
                                      ROTATINGCUBE_SCREEN_HEIGHT,
                                      ROTATINGCUBE_SCREEN_DEPTH, NULL);
    if (!ctx.cubeScreens[1]) {
        writeLog("Error: Could not allocate memory for rotating cube screen 1\n");
        goto __exit_init_cube;
    }

    // Init double buffering - start with buffer 0
    ctx.currentBufferIndex = 0;

    // Load color tables for both screens
    LoadRGB4(&ctx.cubeScreens[0]->ViewPort, ctx.colorTable, ROTATINGCUBE_SCREEN_COLORS);
    LoadRGB4(&ctx.cubeScreens[1]->ViewPort, ctx.colorTable, ROTATINGCUBE_SCREEN_COLORS);

    // Make screen visible
    ScreenToFront(ctx.cubeScreens[ctx.currentBufferIndex]);
    return FSM_ROTATINGCUBE;

__exit_init_cube:
    exitRotatingCube();
    return FSM_ERROR;
}

//----------------------------------------
void exitRotatingCube(void) {
    UBYTE i;
    writeLog("\n== exitRotatingCube() ==\n");

    WaitTOF();
    for (i = 0; i < 2; i++) {
        if (ctx.cubeScreens[i]) {
            CloseScreen(ctx.cubeScreens[i]);
            ctx.cubeScreens[i] = NULL;
        }
    }
    WaitTOF();

    for (i = 0; i < 2; i++) {
        if (ctx.screenBitmaps[i]) {
            FreeBitMap(ctx.screenBitmaps[i]);
            ctx.screenBitmaps[i] = NULL;
        }
    }

    ctx.state = ROTATINGCUBE_INIT;
}
