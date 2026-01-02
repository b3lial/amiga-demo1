#include <exec/types.h>
#include <graphics/gfx.h>
#include <graphics/videocontrol.h>
#include <clib/graphics_protos.h>
#include <clib/exec_protos.h>
#include <hardware/custom.h>
#include <string.h>

#include "showlogo.h"

#include "fsmstates.h"
#include "utils/utils.h"
#include "gfx/stars.h"
#include "gfx/graphicscontroller.h"
#include "gfx/rotation.h"
#include "gfx/zoom.h"
#include "gfx/chunkyconverter.h"
#include "gfx/movementcontroller.h"

struct ShowLogoContext {
    enum ShowLogoState state;
    struct BitMap *logoBitmap;
    struct BitMap *screenBitmaps[2];
    struct Screen *logoscreens[2];
    UWORD dawnPaletteRGB4[256];
    UWORD *color0;
    UBYTE currentBufferIndex;  // 0 or 1
    struct Task *mainTask;     // Main task pointer for signaling
    struct Task *bgTask;       // Background rotation and zoom preparation task
};

static struct ShowLogoContext ctx = {
    .state = SHOWLOGO_INIT,
    .logoBitmap = NULL,
    .screenBitmaps = {NULL, NULL},
    .logoscreens = {NULL, NULL},
    .dawnPaletteRGB4 = {0},
    .color0 = NULL,
    .currentBufferIndex = 0,
    .mainTask = NULL,
    .bgTask = NULL
};

__far extern struct Custom custom;

#define SIGF_PREPARATION_DONE (1L << 16)  // Signal bit for background task completion

//----------------------------------------
/**
 * Background task entry point for preparing rotation and zoom
 *
 * - Allocates memory for rotation bitmaps 
 * - Allocates memory for zoom bitmaps
 * - Generate bitmaps which contain the rotating logo
 * - Generate bitmaps which contain the rotating and scaled bitmaps
 *
 * TODO: Evaluate whether __saveds and -fbaserel is neccessary
 */ 
static void prepareRotationAndZoomTask(void) {
    struct p2cStruct p2c = {0};
    UBYTE destinationBufferIndex = 0;
    UWORD scaleDownFactors[] = {
        // Scale down (18 steps: 1.0 -> 0.36)
        FLOATTOFIX(1.0),   FLOATTOFIX(0.96),  FLOATTOFIX(0.92),  FLOATTOFIX(0.88),
        FLOATTOFIX(0.84),  FLOATTOFIX(0.80),  FLOATTOFIX(0.76),  FLOATTOFIX(0.72),
        FLOATTOFIX(0.68),  FLOATTOFIX(0.64),  FLOATTOFIX(0.60),  FLOATTOFIX(0.56),
        FLOATTOFIX(0.52),  FLOATTOFIX(0.48),  FLOATTOFIX(0.44),  FLOATTOFIX(0.40),
        FLOATTOFIX(0.36),  FLOATTOFIX(0.36),
        // Scale up (18 steps: 0.36 -> 1.0)
        FLOATTOFIX(0.36),  FLOATTOFIX(0.40),  FLOATTOFIX(0.44),  FLOATTOFIX(0.48),
        FLOATTOFIX(0.52),  FLOATTOFIX(0.56),  FLOATTOFIX(0.60),  FLOATTOFIX(0.64),
        FLOATTOFIX(0.68),  FLOATTOFIX(0.72),  FLOATTOFIX(0.76),  FLOATTOFIX(0.80),
        FLOATTOFIX(0.84),  FLOATTOFIX(0.88),  FLOATTOFIX(0.92),  FLOATTOFIX(0.96)
    };
    UWORD scaleDownFactor = 0;

    // Lower our priority to ensure main task gets preferential CPU time
    SetTaskPri(FindTask(NULL), -5);

    //--------- Step 1: Prepare rotation ---------
    if (!startRotationEngine(SHOWLOGO_ROTATION_STEPS, SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT)) {
        goto __exit_prepare_rotation_and_zoom_task;
    }

    // Convert vanilla planar logo to chunky representation
    p2c.bmap = ctx.logoBitmap;
    p2c.startX = 0;
    p2c.startY = 0;
    p2c.width = SHOWLOGO_DAWN_WIDTH;
    p2c.height = SHOWLOGO_DAWN_HEIGHT;
    p2c.chunkybuffer = getRotationSourceBuffer();
    PlanarToChunkyAsm(&p2c);

    // Now that we have the vanilla logo in its chunky format, we can use it to generate rotated versions
    rotateAll();

    //--------- Step 2: Prepare zoom ---------
    if (!startZoomEngine(SHOWLOGO_ROTATION_STEPS, SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT)) {
        goto __exit_prepare_rotation_and_zoom_task;
    }

    // Apply zoom factors to the rotation image sequence
    for (; destinationBufferIndex < SHOWLOGO_ROTATION_STEPS; destinationBufferIndex++) {
        scaleDownFactor = scaleDownFactors[destinationBufferIndex % (sizeof(scaleDownFactors) / sizeof(scaleDownFactors[0]))];
        zoomBitmap(getRotationDestinationBuffer(destinationBufferIndex),
                   scaleDownFactor, destinationBufferIndex);

        // Yield CPU every 4 zoom operations to keep main task responsive
        if ((destinationBufferIndex & 3) == 0) {
            Delay(0);
        }
    }

__exit_prepare_rotation_and_zoom_task:
    // Signal completion to main task (whether successful or failed)
    Signal(ctx.mainTask, SIGF_PREPARATION_DONE);
}

//----------------------------------------
UWORD fsmShowLogo(void) {
    if (mouseClick()) {
        ctx.state = SHOWLOGO_SHUTDOWN;
    }

    switch (ctx.state) {
        case SHOWLOGO_INIT:
            ctx.state = SHOWLOGO_STATIC;
            break;
        case SHOWLOGO_STATIC:
            ctx.state = fadeInFromWhite();
            break;
        case SHOWLOGO_PREPARE_ROTATION_AND_ZOOM:
            ctx.state = prepareRotationAndZoom();
            break;
        case SHOWLOGO_DELAY:
            ctx.state = performDelay();
            break;
        case SHOWLOGO_ROTATE:
            ctx.state = performRotation();
            break;
        case SHOWLOGO_ZOOM:
            ctx.state = performZoom();
            break;
        case SHOWLOGO_SHUTDOWN:
            exitShowLogo();
            return FSM_STOP;
    }

    return FSM_SHOWLOGO;
}

//----------------------------------------
UWORD initShowLogo(void) {
    UWORD i = 0;
    struct Rectangle logoClip;
    writeLog("\n\n== initShowLogo() ==\n");

    // load demo logo from file which we blit later into screenBitmap
    ctx.logoBitmap = loadBlob("img/dawn_224_224_8.RAW", SHOWLOGO_DAWN_DEPTH,
                          SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT);
    if (!ctx.logoBitmap) {
        writeLog("Error: Could not allocate memory for dawn logo bitmap\n");
        goto __exit_init_logo;
    }

    // Load dawn logo color table
    loadColorMap("img/dawn_224_224_8.CMAP", ctx.dawnPaletteRGB4,
                 SHOWLOGO_DAWN_COLORS);

    // load onscreen bitmap which will be shown on screen
    ctx.screenBitmaps[0] = AllocBitMap(SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                                SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER,
                                SHOWLOGO_SCREEN_DEPTH, BMF_DISPLAYABLE | BMF_CLEAR,
                                NULL);
    if (!ctx.screenBitmaps[0]) {
        writeLog("Error: Could not allocate memory for onscreen bitmap 0\n");
        goto __exit_init_logo;
    }

    // load second onscreen bitmap which will be shown on screen for double buffering
    ctx.screenBitmaps[1] = AllocBitMap(SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                                SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER,
                                SHOWLOGO_SCREEN_DEPTH, BMF_DISPLAYABLE | BMF_CLEAR,
                                NULL);
    if (!ctx.screenBitmaps[1]) {
        writeLog("Error: Could not allocate memory for onscreen bitmap 1\n");
        goto __exit_init_logo;
    }

    // this color table will fade from white to logo
    ctx.color0 = AllocVec(sizeof(ctx.dawnPaletteRGB4), 0);
    if (!ctx.color0) {
        writeLog("Error: Could not allocate memory for logo color table\n");
        goto __exit_init_logo;
    }
    for (; i < SHOWLOGO_SCREEN_COLORS; i++) {
        ctx.color0[i] = 0x0fff;
    }

    // create one screen which contains the demo logo
    writeLog("Create screen\n");
    logoClip.MinX = 0;
    logoClip.MinY = 0;
    logoClip.MaxX = SHOWLOGO_SCREEN_WIDTH;
    logoClip.MaxY = SHOWLOGO_SCREEN_HEIGHT;
    ctx.logoscreens[0] = createScreen(ctx.screenBitmaps[0], TRUE,
                               -SHOWLOGO_SCREEN_BORDER, -SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_DEPTH, &logoClip);
    if (!ctx.logoscreens[0]) {
        writeLog("Error: Could not allocate memory for logo screen 0\n");
        goto __exit_init_logo;
    }

    // create second screen which will be used for double buffering
    ctx.logoscreens[1] = createScreen(ctx.screenBitmaps[1], TRUE,
                               -SHOWLOGO_SCREEN_BORDER, -SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_DEPTH, &logoClip);
    if (!ctx.logoscreens[1]) {
        writeLog("Error: Could not allocate memory for logo screen 1\n");
        goto __exit_init_logo;
    }

    // init double buffering - start with buffer 0
    ctx.currentBufferIndex = 0;

    LoadRGB4(&ctx.logoscreens[0]->ViewPort, ctx.color0, SHOWLOGO_SCREEN_COLORS);
    LoadRGB4(&ctx.logoscreens[1]->ViewPort, ctx.color0, SHOWLOGO_SCREEN_COLORS);

    createStars(AMOUNT_OF_STARS, SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
              SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER);

    // Initialize circular movement controller with screen and logo dimensions.
    // This calculates the center point and prepares the circular path coordinates.
    initMovementController(
        SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
        SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER,
        SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT
    );
    WORD logoX, logoY;
    getNextPosition(&logoX, &logoY);

    // blit logo into screenBitmap and delete old bitmap
    BltBitMap(ctx.logoBitmap, 0, 0,
              ctx.screenBitmaps[ctx.currentBufferIndex],
              logoX, logoY,
              SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT,
              0xC0, 0xff, 0);

    paintStars(&ctx.logoscreens[ctx.currentBufferIndex]->RastPort, 42, 70, SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER);

    // make screen great again ;)
    ScreenToFront(ctx.logoscreens[ctx.currentBufferIndex]);
    return FSM_SHOWLOGO;

__exit_init_logo:
    exitShowLogo();
    return FSM_ERROR;
}

//----------------------------------------
void exitShowLogo(void) {
    UBYTE i;
    ULONG receivedSignals;

    // Wait for background task to complete if it has been succesfully started
    if (ctx.bgTask) {
        // Check if signal already received (non-blocking check)
        receivedSignals = SetSignal(0, 0);
        if (!(receivedSignals & SIGF_PREPARATION_DONE)) {
            // Signal not yet received, wait for it
            writeLog("Waiting for background rotation and zoom task to complete...\n");
            Wait(SIGF_PREPARATION_DONE);
            // Clear the signal since the background process has terminated and we dont need it anymore
            SetSignal(0, SIGF_PREPARATION_DONE);
        }
        // Task will terminate itself after signaling, just clear the pointer
        ctx.bgTask = NULL;
    }

    WaitTOF();
    for (i = 0; i < 2; i++) {
        if (ctx.logoscreens[i]) {
            CloseScreen(ctx.logoscreens[i]);
            ctx.logoscreens[i] = NULL;
        }
    }
    WaitTOF();

    if (ctx.logoBitmap) {
        FreeBitMap(ctx.logoBitmap);
        ctx.logoBitmap = NULL;
    }

    for (i = 0; i < 2; i++) {
        if (ctx.screenBitmaps[i]) {
            FreeBitMap(ctx.screenBitmaps[i]);
            ctx.screenBitmaps[i] = NULL;
        }
    }

    if (ctx.color0) {
        FreeVec(ctx.color0);
        ctx.color0 = NULL;
    }

    exitRotationEngine();

    exitZoomEngine();
}

//----------------------------------------
UWORD fadeInFromWhite(void) {
    UWORD decrementer;
    UWORD i = 0;
    BOOL fade = FALSE;

    // fade effect on color table
    for (; i < SHOWLOGO_SCREEN_COLORS; i++) {
        decrementer = 0;
        if ((ctx.color0[i] & 0x000f) != (ctx.dawnPaletteRGB4[i] & 0x000f)) {
            decrementer |= 0x0001;
            fade = TRUE;
        }
        if ((ctx.color0[i] & 0x00f0) != (ctx.dawnPaletteRGB4[i] & 0x00f0)) {
            decrementer |= 0x0010;
            fade = TRUE;
        }
        if ((ctx.color0[i] & 0x0f00) != (ctx.dawnPaletteRGB4[i] & 0x0f00)) {
            decrementer |= 0x0100;
            fade = TRUE;
        }
        ctx.color0[i] -= decrementer;
    }

    // update screen and show result of fade in step
    WaitTOF();
    WaitBOVP(&ctx.logoscreens[0]->ViewPort);
    LoadRGB4(&ctx.logoscreens[0]->ViewPort, ctx.color0, SHOWLOGO_SCREEN_COLORS);
    LoadRGB4(&ctx.logoscreens[1]->ViewPort, ctx.color0, SHOWLOGO_SCREEN_COLORS);

    if (!fade) {
        return SHOWLOGO_PREPARE_ROTATION_AND_ZOOM;
    } else {
        return SHOWLOGO_STATIC;
    }
}

//----------------------------------------
UWORD prepareRotationAndZoom(void) {
    // Get main task pointer for signaling
    ctx.mainTask = FindTask(NULL);

    // Clear any pending rotation and zoom preparation signal
    SetSignal(0, SIGF_PREPARATION_DONE);

    // Create background task for rotation and zoom preparation
    ctx.bgTask = (struct Task *)CreateTask((CONST_STRPTR)"PrepareRotationAndZoom", 0, (APTR)prepareRotationAndZoomTask, 4096);

    if (!ctx.bgTask) {
        writeLog("Error: Could not create background rotation and zoom task\n");
        return SHOWLOGO_SHUTDOWN;
    }

    // Transition to delay state where we'll wait for background task
    return SHOWLOGO_DELAY;
}

//----------------------------------------
UWORD paint(UBYTE *sourceChunkyBuffer, BOOL useStaticPosition) {
    UWORD p;
    struct BitMap *bitmap;
    ULONG bytesPerRow;
    WORD logoX, logoY;

    // Convert chunky buffer to bitmap if provided
    if (sourceChunkyBuffer) {
        convertChunkyToBitmap(sourceChunkyBuffer, ctx.logoBitmap);
    }

    switchScreenData();

    // Clear the entire screen bitmap before drawing
    bitmap = ctx.screenBitmaps[ctx.currentBufferIndex];
    bytesPerRow = bitmap->BytesPerRow;
    for (p = 0; p < bitmap->Depth; p++) {
        memset(bitmap->Planes[p], 0, bytesPerRow * (SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER));
    }

    // Get logo position - either static (initial) or moving (next along path)
    if (useStaticPosition) {
        getInitialPosition(&logoX, &logoY);
    } else {
        getNextPosition(&logoX, &logoY);
    }

    // Blit the logo
    BltBitMap(ctx.logoBitmap, 0, 0,
              ctx.screenBitmaps[ctx.currentBufferIndex],
              logoX, logoY,
              SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT,
              0xC0, 0xff, 0);

    // Animate the starfield background
    moveStars(AMOUNT_OF_STARS);
    paintStars(&ctx.logoscreens[ctx.currentBufferIndex]->RastPort, 42, 70,
               SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
               SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER);

    WaitTOF();
    WaitTOF();
    WaitTOF();
    ScreenToFront(ctx.logoscreens[ctx.currentBufferIndex]);

    // Return current position index
    return getCurrentPositionIndex();
}

//----------------------------------------
UWORD performDelay() {
    static UWORD frameCounter = 0;
    ULONG receivedSignals;

    // Continue painting the static logo and animating stars
    paint(NULL, TRUE);

    frameCounter++;

    // Check if background rotation and zoom task has completed (non-blocking)
    receivedSignals = SetSignal(0, 0);  // Read current signals without changing them
    if (receivedSignals & SIGF_PREPARATION_DONE) {
        // Background task finished - transition to rotation after delay
        if (frameCounter >= ONE_SECOND) {
            frameCounter = 0;
            return SHOWLOGO_ROTATE;
        }
    }
    // If background task is still running, just keep waiting and animating

    return SHOWLOGO_DELAY;
}

//----------------------------------------
UWORD performRotation() {
    static UBYTE i = 1;
    static UWORD frameCounter = 0;
    UWORD positionIndex;

    positionIndex = paint(getRotationDestinationBuffer(i), FALSE);

    i = (i < SHOWLOGO_ROTATION_STEPS - 1) ? i + 1 : 0;
    frameCounter++;

    // After a few seconds of rotation AND when back at starting position, switch to zoom
    if ((frameCounter >= ONE_SECOND * 2) && (positionIndex == 0)) {
        frameCounter = 0;
        return SHOWLOGO_ZOOM;
    }

    return SHOWLOGO_ROTATE;
}

//----------------------------------------
UWORD performZoom() {
    static UBYTE i = 1;

    paint(getZoomDestinationBuffer(i), FALSE);

    i = (i < SHOWLOGO_ROTATION_STEPS - 1) ? i + 1 : 0;
    return SHOWLOGO_ZOOM;
}

//----------------------------------------
void convertChunkyToBitmap(UBYTE *sourceChunky, struct BitMap *destPlanar) {
#ifdef NATIVE_CONVERTER
    struct RastPort rastPort1 = {0};
    struct RastPort rastPort2 = {0};
    InitRastPort(&rastPort1);
    InitRastPort(&rastPort2);

    rastPort1.BitMap = destPlanar;
    rastPort2.Layer = NULL;
    rastPort2.BitMap = tempBitmap;
    WritePixelArray8(&rastPort1, 0, 0, SHOWLOGO_DAWN_WIDTH - 1,
                     SHOWLOGO_DAWN_HEIGHT - 1, sourceChunky, &rastPort2);
#else
    struct c2pStruct c2p;
    c2p.bmap = destPlanar;
    c2p.startX = 0;
    c2p.startY = 0;
    c2p.width = SHOWLOGO_DAWN_WIDTH;
    c2p.height = SHOWLOGO_DAWN_HEIGHT;
    c2p.chunkybuffer = sourceChunky;
    ChunkyToPlanarAsm(&c2p);
#endif
}

//----------------------------------------
void switchScreenData() {
    // Flip between buffer 0 and 1
    ctx.currentBufferIndex = 1 - ctx.currentBufferIndex;
}
