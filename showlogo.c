#include "demo1.h"

WORD payloadShowLogoState = SHOWLOGO_INIT;
struct BitMap *logo = NULL;
struct BitMap *logoWithBorders;
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

    // load demo logo from file which we blit later into logoWithBorders
    logo = loadBlob("img/dawn2_240_201_8.RAW", SHOWLOGO_DAWN_DEPTH,
                    SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT);
    if (!logo) {
        writeLog("Error: Could not allocate memory for dawn logo bitmap\n");
        goto __exit_init_logo;
    }

    // Load dawn logo color table
    loadColorMap("img/dawn2_240_201_8.CMAP", dawnPaletteRGB4,
                 SHOWLOGO_DAWN_COLORS);

    // load onscreen bitmap which will be shown on screen
    logoWithBorders = AllocBitMap(SHOWLOGO_BLOB_WIDTH + SHOWLOGO_BLOB_BORDER,
                                  SHOWLOGO_BLOB_HEIGHT + SHOWLOGO_BLOB_BORDER,
                                  SHOWLOGO_BLOB_DEPTH, BMF_DISPLAYABLE | BMF_CLEAR,
                                  NULL);
    if (!logoWithBorders) {
        writeLog("Error: Could not allocate memory for onscreen bitmap\n");
        goto __exit_init_logo;
    }

    // blit logo into logoWithBorders and delete old bitmap
    BltBitMap(logo, 0, 0,
              logoWithBorders,
              SHOWLOGO_BLOB_BORDER, SHOWLOGO_BLOB_BORDER,
              SHOWLOGO_DAWN_WIDTH, SHOWLOGO_DAWN_HEIGHT,
              0xC0, 0xff, 0);
    FreeBitMap(logo);
    logo = NULL;

    // this color table will fade from white to logo
    color0 = AllocVec(sizeof(dawnPaletteRGB4), NULL);
    if (!color0) {
        writeLog("Error: Could not allocate memory for logo color table\n");
        goto __exit_init_logo;
    }
    for (; i < SHOWLOGO_BLOB_COLORS; i++) {
        color0[i] = 0x0fff;
    }

    // create one screen which contains the demo logo
    writeLog("Create screen\n");
    logoClip.MinX = 0;
    logoClip.MinY = 0;
    logoClip.MaxX = SHOWLOGO_BLOB_WIDTH;
    logoClip.MaxY = SHOWLOGO_BLOB_HEIGHT;
    logoscreen0 = createScreen(logoWithBorders, TRUE,
                               -SHOWLOGO_BLOB_BORDER, -SHOWLOGO_BLOB_BORDER,
                               SHOWLOGO_BLOB_WIDTH_WITH_BORDER,
                               SHOWLOGO_BLOB_HEIGHT_WITH_BORDER,
                               SHOWLOGO_BLOB_DEPTH, &logoClip);
    if (!logoscreen0) {
        writeLog("Error: Could not allocate memory for logo screen\n");
        goto __exit_init_logo;
    }
    LoadRGB4(&logoscreen0->ViewPort, color0, SHOWLOGO_BLOB_COLORS);

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

    if (logo) {
        FreeBitMap(logo);
        logo = NULL;
    }
    if (logoWithBorders) {
        FreeBitMap(logoWithBorders);
        logo = NULL;
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
    for (; i < SHOWLOGO_BLOB_COLORS; i++) {
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
    LoadRGB4(&logoscreen0->ViewPort, color0, SHOWLOGO_BLOB_COLORS);
}