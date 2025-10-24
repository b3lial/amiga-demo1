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
#include "gfx/chunkyconverter.h"

struct ShowLogoContext {
    enum ShowLogoState state;
    struct BitMap *logoBitmap;
    struct BitMap *screenBitmaps[2];
    struct Screen *logoscreens[2];
    UWORD dawnPaletteRGB4[256];
    UWORD *color0;
    UBYTE currentBufferIndex;  // 0 or 1
};

static struct ShowLogoContext ctx = {
    .state = SHOWLOGO_INIT,
    .logoBitmap = NULL,
    .screenBitmaps = {NULL, NULL},
    .logoscreens = {NULL, NULL},
    .dawnPaletteRGB4 = {0},
    .color0 = NULL,
    .currentBufferIndex = 0
};

__far extern struct Custom custom;

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
        case SHOWLOGO_PREPARE_ROTATION:
            ctx.state = prepareRotation();
            break;
        case SHOWLOGO_DELAY:
            Delay(THREE_SECONDS);
            ctx.state = SHOWLOGO_ROTATE;
            break;
        case SHOWLOGO_ROTATE:
            ctx.state = performRotation();
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

    createStars(70, SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
              SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER);

    // blit logo into screenBitmap and delete old bitmap
    BltBitMap(ctx.logoBitmap, 0, 0,
              ctx.screenBitmaps[ctx.currentBufferIndex],
              SHOWLOGO_DAWN_X_POS, SHOWLOGO_DAWN_Y_POS,
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
        return SHOWLOGO_PREPARE_ROTATION;
    } else {
        return SHOWLOGO_STATIC;
    }
}

//----------------------------------------
UWORD prepareRotation(void) {
    struct p2cStruct p2c = {0};

    // allocate source buffer and destination buffer array
    if (!startRotationEngine(SHOWLOGO_ROTATION_STEPS, SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT)) {
        return SHOWLOGO_SHUTDOWN;
    }

    // convert planar buffer to chunky
    p2c.bmap = ctx.logoBitmap;
    p2c.startX = 0;
    p2c.startY = 0;
    p2c.width = SHOWLOGO_DAWN_WIDTH;
    p2c.height = SHOWLOGO_DAWN_HEIGHT;
    p2c.chunkybuffer = getSourceBuffer();
    PlanarToChunkyAsm(&p2c);

    rotateAll();
    return SHOWLOGO_DELAY;
}

//----------------------------------------
UWORD performRotation() {
    static UBYTE i = 1;
    UWORD p;
    struct BitMap *bitmap;
    ULONG bytesPerRow;

    convertChunkyToBitmap(getDestBuffer(i), ctx.logoBitmap);

    switchScreenData();

    // Clear the entire screen bitmap before drawing
    bitmap = ctx.screenBitmaps[ctx.currentBufferIndex];
    bytesPerRow = bitmap->BytesPerRow;
    for (p = 0; p < bitmap->Depth; p++) {
        memset(bitmap->Planes[p], 0, bytesPerRow * (SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER));
    }

    BltBitMap(ctx.logoBitmap, 0, 0,
              ctx.screenBitmaps[ctx.currentBufferIndex],
              SHOWLOGO_DAWN_X_POS, SHOWLOGO_DAWN_Y_POS,
              SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT,
              0xC0, 0xff, 0);

    moveStars(STAR_MAX);
    paintStars(&ctx.logoscreens[ctx.currentBufferIndex]->RastPort, 42, 70, SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER);

    WaitTOF();
    WaitTOF();
    WaitTOF();
    ScreenToFront(ctx.logoscreens[ctx.currentBufferIndex]);

    i = (i < SHOWLOGO_ROTATION_STEPS - 1) ? i + 1 : 0;
    return SHOWLOGO_ROTATE;
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
