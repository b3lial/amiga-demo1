#ifndef __PLASMA_H__
#define __PLASMA_H__

#include <exec/types.h>

enum PlasmaState {
    PLASMA_INIT = 0,
    PLASMA_ANIMATE = 1,
    PLASMA_EXIT = 2
};

UWORD fsmPlasma(void);
UWORD initPlasma(void);
void exitPlasma(void);

#endif
