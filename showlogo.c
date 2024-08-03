#include "demo1.h"

WORD payloadShowLogoState = SHOWLOGO_INIT;
struct BitMap *logoBitmap = NULL;
struct BitMap *screenBitmap0;
struct BitMap *screenBitmap1;
struct Screen *logoscreen0 = NULL;
struct Screen *logoscreen1 = NULL;
UWORD dawnPaletteRGB4[256] = {0};
UWORD *color0 = NULL;

// used for double buffering
BOOL bufferSelector;
struct BitMap *currentBitmap = NULL;
struct Screen *currentScreen = NULL;

__far extern struct Custom custom;

UWORD fsmShowLogo(void) {
    if (mouseClick()) {
        payloadShowLogoState = SHOWLOGO_SHUTDOWN;
    }

    switch (payloadShowLogoState) {
        case SHOWLOGO_INIT:
            payloadShowLogoState = SHOWLOGO_STATIC;
            break;
        case SHOWLOGO_STATIC:
            payloadShowLogoState = fadeInFromWhite();
            break;
        case SHOWLOGO_PREPARE_ROTATION:
            payloadShowLogoState = prepareRotation();
            break;
        case SHOWLOGO_ROTATE:
            payloadShowLogoState = performRotation();
            break;
        case SHOWLOGO_SHUTDOWN:
            exitShowLogo();
            return FSM_STOP;
    }

    return FSM_SHOWLOGO;
}

/* We create two bitmaps here: the bitmap which is shown on the
 * screen and the image blob. The onscreen bitmap has a size
 * of image blob + border. The image blob is blitted into
 * the middle of the oncreen bitmap. This allows rotation effects
 * without bothering any array out of bounds problems
 */
UWORD initShowLogo(void) {
    UWORD i = 0;
    struct Rectangle logoClip;
    writeLog("\n\n== initShowLogo() ==\n");

    // load demo logo from file which we blit later into screenBitmap
    logoBitmap = loadBlob("img/dawn_224_224_8.RAW", SHOWLOGO_DAWN_DEPTH,
                          SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT);
    if (!logoBitmap) {
        writeLog("Error: Could not allocate memory for dawn logo bitmap\n");
        goto __exit_init_logo;
    }

    // Load dawn logo color table
    loadColorMap("img/dawn_224_224_8.CMAP", dawnPaletteRGB4,
                 SHOWLOGO_DAWN_COLORS);

    // load onscreen bitmap which will be shown on screen
    screenBitmap0 = AllocBitMap(SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                                SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER,
                                SHOWLOGO_SCREEN_DEPTH, BMF_DISPLAYABLE | BMF_CLEAR,
                                NULL);
    if (!screenBitmap0) {
        writeLog("Error: Could not allocate memory for onscreen bitmap 0\n");
        goto __exit_init_logo;
    }

    // load second onscreen bitmap which will be shown on screen for double buffering
    screenBitmap1 = AllocBitMap(SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                                SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER,
                                SHOWLOGO_SCREEN_DEPTH, BMF_DISPLAYABLE | BMF_CLEAR,
                                NULL);
    if (!screenBitmap1) {
        writeLog("Error: Could not allocate memory for onscreen bitmap 0\n");
        goto __exit_init_logo;
    }

    // this color table will fade from white to logo
    color0 = AllocVec(sizeof(dawnPaletteRGB4), NULL);
    if (!color0) {
        writeLog("Error: Could not allocate memory for logo color table\n");
        goto __exit_init_logo;
    }
    for (; i < SHOWLOGO_SCREEN_COLORS; i++) {
        color0[i] = 0x0fff;
    }

    // create one screen which contains the demo logo
    writeLog("Create screen\n");
    logoClip.MinX = 0;
    logoClip.MinY = 0;
    logoClip.MaxX = SHOWLOGO_SCREEN_WIDTH;
    logoClip.MaxY = SHOWLOGO_SCREEN_HEIGHT;
    logoscreen0 = createScreen(screenBitmap0, TRUE,
                               -SHOWLOGO_SCREEN_BORDER, -SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_DEPTH, &logoClip);
    if (!logoscreen0) {
        writeLog("Error: Could not allocate memory for logo screen 0\n");
        goto __exit_init_logo;
    }

    // create second screen which will be used for double buffering
    logoscreen1 = createScreen(screenBitmap1, TRUE,
                               -SHOWLOGO_SCREEN_BORDER, -SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_DEPTH, &logoClip);
    if (!logoscreen1) {
        writeLog("Error: Could not allocate memory for logo screen 1\n");
        goto __exit_init_logo;
    }

    // init double buffering
    bufferSelector = TRUE;
    currentScreen = logoscreen0;  // main screen turn on ;)
    currentBitmap = screenBitmap0;

    LoadRGB4(&logoscreen0->ViewPort, color0, SHOWLOGO_SCREEN_COLORS);
    LoadRGB4(&logoscreen1->ViewPort, color0, SHOWLOGO_SCREEN_COLORS);
    createStars(&logoscreen0->RastPort, 42, 70, SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER);
    createStars(&logoscreen1->RastPort, 42, 70, SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER);

    // blit logo into screenBitmap and delete old bitmap
    BltBitMap(logoBitmap, 0, 0,
              currentBitmap,
              SHOWLOGO_DAWN_X_POS, SHOWLOGO_DAWN_Y_POS,
              SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT,
              0xC0, 0xff, 0);

    // make screen great again ;)
    ScreenToFront(currentScreen);
    return FSM_SHOWLOGO;

__exit_init_logo:
    exitShowLogo();
    return FSM_ERROR;
}

void exitShowLogo(void) {
    WaitTOF();
    if (logoscreen0) {
        CloseScreen(logoscreen0);
        logoscreen0 = NULL;
    }
    if (logoscreen1) {
        CloseScreen(logoscreen1);
        logoscreen1 = NULL;
    }
    WaitTOF();

    if (logoBitmap) {
        FreeBitMap(logoBitmap);
        logoBitmap = NULL;
    }
    if (screenBitmap0) {
        FreeBitMap(screenBitmap0);
        screenBitmap0 = NULL;
    }
    if (screenBitmap1) {
        FreeBitMap(screenBitmap1);
        screenBitmap1 = NULL;
    }
    if (color0) {
        FreeVec(color0);
        color0 = NULL;
    }

    freeRotationEngine();
}

UWORD fadeInFromWhite(void) {
    UWORD decrementer;
    UWORD i = 0;
    BOOL fade = FALSE;

    // fade effect on color table
    for (; i < SHOWLOGO_SCREEN_COLORS; i++) {
        decrementer = 0;
        if ((color0[i] & 0x000f) != (dawnPaletteRGB4[i] & 0x000f)) {
            decrementer |= 0x0001;
            fade = TRUE;
        }
        if ((color0[i] & 0x00f0) != (dawnPaletteRGB4[i] & 0x00f0)) {
            decrementer |= 0x0010;
            fade = TRUE;
        }
        if ((color0[i] & 0x0f00) != (dawnPaletteRGB4[i] & 0x0f00)) {
            decrementer |= 0x0100;
            fade = TRUE;
        }
        color0[i] -= decrementer;
    }

    // update screen and show result of fade in step
    WaitTOF();
    WaitBOVP(&logoscreen0->ViewPort);
    LoadRGB4(&logoscreen0->ViewPort, color0, SHOWLOGO_SCREEN_COLORS);
    LoadRGB4(&logoscreen1->ViewPort, color0, SHOWLOGO_SCREEN_COLORS);

    if (!fade) {
        return SHOWLOGO_PREPARE_ROTATION;
    } else {
        return SHOWLOGO_STATIC;
    }
}

UWORD prepareRotation(void) {
    struct p2cStruct p2c = {0};

    // allocate source buffer and destination buffer array
    if (!initRotationEngine(SHOWLOGO_ROTATION_STEPS, SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT)) {
        return SHOWLOGO_SHUTDOWN;
    }

    // convert planar buffer to chunky
    p2c.bmap = logoBitmap;
    p2c.startX = 0;
    p2c.startY = 0;
    p2c.width = SHOWLOGO_DAWN_WIDTH;
    p2c.height = SHOWLOGO_DAWN_HEIGHT;
    p2c.chunkybuffer = getSourceBuffer();
    PlanarToChunkyAsm(&p2c);

    rotateAll();
    return SHOWLOGO_ROTATE;
}

UWORD performRotation() {
    static UBYTE i = 1;
    convertChunkyToBitmap(getDestBuffer(i), logoBitmap);

    switchScreenData();
    BltBitMap(logoBitmap, 0, 0,
              currentBitmap,
              SHOWLOGO_DAWN_X_POS, SHOWLOGO_DAWN_Y_POS,
              SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT,
              0xC0, 0xff, 0);

    WaitTOF();
    WaitTOF();
    WaitTOF();
    ScreenToFront(currentScreen);

    i = (i < SHOWLOGO_ROTATION_STEPS - 1) ? i + 1 : 0;
    return SHOWLOGO_ROTATE;
}

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

void switchScreenData() {
    if (bufferSelector) {
        currentScreen = logoscreen1;
        currentBitmap = screenBitmap1;
        bufferSelector = FALSE;
    } else {
        currentScreen = logoscreen0;
        currentBitmap = screenBitmap0;
        bufferSelector = TRUE;
    }
}
