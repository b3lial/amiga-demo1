#ifndef __PLASMA_H__
#define __PLASMA_H__

#include <exec/types.h>

enum PlasmaState {
    PLASMA_INIT = 0,
    PLASMA_ANIMATE = 1,
    PLASMA_EXIT = 2
};

#define PLASMA_SCREEN_WIDTH  320
#define PLASMA_SCREEN_HEIGHT 256
#define PLASMA_SCREEN_DEPTH  8
#define PLASMA_SCREEN_COLORS 256

// Plasma is computed at half resolution and scaled up 2x by the blitter.
// Width must be a multiple of 32 so the c2p takes its fast longword path.
#define PLASMA_CHUNKY_WIDTH  (PLASMA_SCREEN_WIDTH / 2)
#define PLASMA_CHUNKY_HEIGHT (PLASMA_SCREEN_HEIGHT / 2)

UWORD fsmPlasma(void);
UWORD initPlasma(void);
void exitPlasma(void);

#endif
