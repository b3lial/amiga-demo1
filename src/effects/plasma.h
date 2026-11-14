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

UWORD fsmPlasma(void);
UWORD initPlasma(void);
void exitPlasma(void);

#endif
