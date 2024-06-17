#ifndef __SHOW_LOGO_H__
#define __SHOW_LOGO_H__

#define SHOWLOGO_INIT 0
#define SHOWLOGO_STATIC 1
#define SHOWLOGO_PREPARE_ROTATION 2
#define SHOWLOGO_ROTATE 3
#define SHOWLOGO_SHUTDOWN 4

#define SHOWLOGO_ROTATION_STEPS 36

#define SHOWLOGO_SCREEN_BORDER 20
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

UWORD fsmShowLogo(void);
UWORD initShowLogo(void);
void exitShowLogo(void);
UWORD fadeInFromWhite(void);
UWORD prepareRotation(void);
UWORD performRotation(void);

#endif
