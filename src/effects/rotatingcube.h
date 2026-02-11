#ifndef __ROTATINGCUBE_H__
#define __ROTATINGCUBE_H__

#include <exec/types.h>
#include "gfx/fixedpoint.h"

enum RotatingCubeState {
    ROTATINGCUBE_INIT = 0,
    ROTATINGCUBE_RUNNING = 1,
    ROTATINGCUBE_SHUTDOWN = 2
};

#define ROTATINGCUBE_SCREEN_WIDTH 320
#define ROTATINGCUBE_SCREEN_HEIGHT 256
#define ROTATINGCUBE_SCREEN_DEPTH 4
#define ROTATINGCUBE_SCREEN_COLORS 16

// Scene constants (fixed-point)
#define CUBE_CENTER_X FLOATTOFIX(0.0)
#define CUBE_CENTER_Y FLOATTOFIX(0.0)
#define CUBE_CENTER_Z FLOATTOFIX(3.0)
#define CUBE_HALF_SIZE FLOATTOFIX(1.0)
#define SCREEN_PLANE_Z FLOATTOFIX(1.0)

// 3D vector using fixed-point arithmetic
struct Vec3 {
    WORD x;
    WORD y;
    WORD z;
};

// Ray direction vector (origin is shared across all rays, stored in context)
// NOTE: Direction vectors are NOT normalized (for performance)
// Ray intersection code must handle unnormalized directions
typedef struct Vec3 RayDirection;

UWORD fsmRotatingCube(void);
UWORD initRotatingCube(void);
void exitRotatingCube(void);

#endif
