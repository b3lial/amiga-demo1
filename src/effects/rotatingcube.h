#ifndef __ROTATINGCUBE_H__
#define __ROTATINGCUBE_H__

#include <exec/types.h>

enum RotatingCubeState {
    ROTATINGCUBE_INIT = 0,
    ROTATINGCUBE_RUNNING = 1,
    ROTATINGCUBE_SHUTDOWN = 2
};

#define ROTATINGCUBE_SCREEN_WIDTH 320
#define ROTATINGCUBE_SCREEN_HEIGHT 256
#define ROTATINGCUBE_SCREEN_DEPTH 4
#define ROTATINGCUBE_SCREEN_COLORS 16

UWORD fsmRotatingCube(void);
UWORD initRotatingCube(void);
void exitRotatingCube(void);

#endif
