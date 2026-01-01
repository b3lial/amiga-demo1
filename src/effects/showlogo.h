#ifndef __SHOW_LOGO_H__
#define __SHOW_LOGO_H__

#include <exec/types.h>

enum ShowLogoState {
    SHOWLOGO_INIT = 0,
    SHOWLOGO_STATIC = 1,
    SHOWLOGO_PREPARE_ROTATION = 2,
    SHOWLOGO_PREPARE_ZOOM = 3,
    SHOWLOGO_DELAY = 4,
    SHOWLOGO_ROTATE = 5,
    SHOWLOGO_ZOOM = 6,
    SHOWLOGO_SHUTDOWN = 7
};

#define SHOWLOGO_ROTATION_STEPS 36

#define SHOWLOGO_SCREEN_BORDER 30
#define SHOWLOGO_SCREEN_DEPTH 8
#define SHOWLOGO_SCREEN_COLORS 256
#define SHOWLOGO_SCREEN_WIDTH 320
#define SHOWLOGO_SCREEN_HEIGHT 256

#define SHOWLOGO_DAWN_DEPTH 8
#define SHOWLOGO_DAWN_COLORS 256
#define SHOWLOGO_DAWN_WIDTH 224
#define SHOWLOGO_DAWN_HEIGHT 224

#define SHOWLOGO_DAWN_X_POS \
    (((SHOWLOGO_SCREEN_WIDTH - SHOWLOGO_DAWN_WIDTH) / 2) + SHOWLOGO_SCREEN_BORDER)
#define SHOWLOGO_DAWN_Y_POS \
    (((SHOWLOGO_SCREEN_HEIGHT - SHOWLOGO_DAWN_HEIGHT) / 2) + SHOWLOGO_SCREEN_BORDER)

#define ONE_SECOND 50
#define TWO_SECONDS 100
#define AMOUNT_OF_STARS 70

UWORD fsmShowLogo(void);
UWORD initShowLogo(void);
void exitShowLogo(void);
UWORD fadeInFromWhite(void);
UWORD prepareRotation(void);
UWORD prepareZoom(void);
/**
 * Convert the current rotated frame from planar to chunky and blit it on screen.
 * The chunky2planar conversion needs to be done per frame because otherwise
 * chip memory would be exhausted easily.
 */
UWORD performRotation(void);
UWORD performZoom(void);
UWORD paint(UBYTE *sourceChunkyBuffer);
void switchScreenData(void);
void convertChunkyToBitmap(UBYTE* sourceChunky, struct BitMap* destPlanar);

#endif
