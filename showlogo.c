#include "demo1.h"

WORD payloadShowLogoState = SHOWLOGO_INIT;
struct BitMap *logoBitmap = NULL;
struct BitMap *screenBitmap;
struct Screen *logoscreen0 = NULL;
UWORD dawnPaletteRGB4[256] = {0};
UWORD *color0 = NULL;

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
            fadeInFromWhite();
            break;
        case SHOWLOGO_SHUTDOWN:
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
    logoBitmap = loadBlob("img/dawn2_240_201_8.RAW", SHOWLOGO_DAWN_DEPTH,
                          SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT);
    if (!logoBitmap) {
        writeLog("Error: Could not allocate memory for dawn logo bitmap\n");
        goto __exit_init_logo;
    }

    // Load dawn logo color table
    loadColorMap("img/dawn2_240_201_8.CMAP", dawnPaletteRGB4,
                 SHOWLOGO_DAWN_COLORS);

    // load onscreen bitmap which will be shown on screen
    screenBitmap = AllocBitMap(SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_DEPTH, BMF_DISPLAYABLE | BMF_CLEAR,
                               NULL);
    if (!screenBitmap) {
        writeLog("Error: Could not allocate memory for onscreen bitmap\n");
        goto __exit_init_logo;
    }

    // blit logo into screenBitmap and delete old bitmap
    BltBitMap(logoBitmap, 0, 0,
              screenBitmap,
              SHOWLOGO_DAWN_X_POS, SHOWLOGO_DAWN_Y_POS,
              SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT,
              0xC0, 0xff, 0);
    FreeBitMap(logoBitmap);
    logoBitmap = NULL;

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
    logoscreen0 = createScreen(screenBitmap, TRUE,
                               -SHOWLOGO_SCREEN_BORDER, -SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_WIDTH + SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_HEIGHT + SHOWLOGO_SCREEN_BORDER,
                               SHOWLOGO_SCREEN_DEPTH, &logoClip);
    if (!logoscreen0) {
        writeLog("Error: Could not allocate memory for logo screen\n");
        goto __exit_init_logo;
    }
    LoadRGB4(&logoscreen0->ViewPort, color0, SHOWLOGO_SCREEN_COLORS);

    // make screen great again ;)
    ScreenToFront(logoscreen0);
    return FSM_SHOWLOGO;

__exit_init_logo:
    exitShowLogo();
    return FSM_ERROR;
}

void exitShowLogo(void) {
    if (logoscreen0) {
        CloseScreen(logoscreen0);
        logoscreen0 = NULL;
    }
    WaitTOF();

    if (logoBitmap) {
        FreeBitMap(logoBitmap);
        logoBitmap = NULL;
    }
    if (screenBitmap) {
        FreeBitMap(screenBitmap);
        logoBitmap = NULL;
    }
    if (color0) {
        FreeVec(color0);
        color0 = NULL;
    }
}

void fadeInFromWhite(void) {
    UWORD decrementer;
    UWORD i = 0;

    // fade effect on color table
    for (; i < SHOWLOGO_SCREEN_COLORS; i++) {
        decrementer = 0;
        if ((color0[i] & 0x000f) != (dawnPaletteRGB4[i] & 0x000f)) {
            decrementer |= 0x0001;
        }
        if ((color0[i] & 0x00f0) != (dawnPaletteRGB4[i] & 0x00f0)) {
            decrementer |= 0x0010;
        }
        if ((color0[i] & 0x0f00) != (dawnPaletteRGB4[i] & 0x0f00)) {
            decrementer |= 0x0100;
        }
        color0[i] -= decrementer;
    }

    // update screen and show result of fade in step
    WaitTOF();
    WaitBOVP(&logoscreen0->ViewPort);
    LoadRGB4(&logoscreen0->ViewPort, color0, SHOWLOGO_SCREEN_COLORS);
}