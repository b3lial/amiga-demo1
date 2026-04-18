#ifndef __ROTATINGCUBE_H__
#define __ROTATINGCUBE_H__

#include <exec/types.h>
#include "gfx/fixedpoint.h"

enum RotatingCubeState {
    ROTATINGCUBE_INIT = 0,
    ROTATINGCUBE_PREPARE = 1,
    ROTATINGCUBE_WAIT_PREPARE = 2,
    ROTATINGCUBE_RUNNING = 3,
    ROTATINGCUBE_SHUTDOWN = 4
};

#define ROTATINGCUBE_SCREEN_WIDTH 320
#define ROTATINGCUBE_SCREEN_HEIGHT 256
#define ROTATINGCUBE_SCREEN_DEPTH 5
#define ROTATINGCUBE_SCREEN_COLORS 32

// Rotation parameters
#define ROTATION_STEPS 36  // 0° to 350° in 10° increments (360° == 0°)
#define DEGREE_RESOLUTION 10  // 10 degrees per step

// Scene constants (fixed-point)
#define CUBE_CENTER_X FLOATTOFIX(0.0)
#define CUBE_CENTER_Y FLOATTOFIX(0.0)
#define CUBE_CENTER_Z FLOATTOFIX(3.0)
#define CUBE_HALF_SIZE FLOATTOFIX(1.0)
#define SCREEN_PLANE_Z FLOATTOFIX(1.0)

// Pixel aspect ratio correction for Amiga PAL 320x256
// Adjust this value to make the cube appear square on your display
// Values to try: 1.0 (no correction), 1.2, 1.3, 1.4, 1.5
#define PIXEL_ASPECT_RATIO FLOATTOFIX(1.5)

// Cube related stuff
#define CUBE_COLORS (ROTATINGCUBE_SCREEN_COLORS - 2)
#define CUBE_MAX_DISTANCE FLOATTOFIX(3.45)
#define CUBE_MIN_DISTANCE FLOATTOFIX(1.25)
#define CUBE_EXTENT (CUBE_MAX_DISTANCE - CUBE_MIN_DISTANCE)
#define CUBE_COLOR_SECTION_SIZE (FIXDIV(CUBE_EXTENT, INTTOFIX(CUBE_COLORS)))

// Border: defines how many pixels to skip on each side when rendering the cube.
// The cube is rendered into a smaller inner region to save CPU and RAM.
// NDC calculation still uses the full screen dimensions for correct projection.
#define BORDER_WIDTH  80
#define BORDER_HEIGHT 43
#define CUBE_INNER_WIDTH  (ROTATINGCUBE_SCREEN_WIDTH  - 2 * BORDER_WIDTH)
#define CUBE_INNER_HEIGHT (ROTATINGCUBE_SCREEN_HEIGHT - 2 * BORDER_HEIGHT)

#define SIGF_CUBE_PREPARE_DONE (1L << 17)  // Signal bit for cube raytracing task completion (bit 16 is used by showlogo)

// Background stuff
#define GRID_SPACING 32
#define GRID_DIRECTION_FRAMES 80  // frames before a new random direction is picked

// 3D vector using fixed-point arithmetic
struct Vec3 {
    fix16 x;
    fix16 y;
    fix16 z;
};

// Ray direction vector (origin is shared across all rays, stored in context)
// NOTE: Direction vectors are NOT normalized (for performance)
// Ray intersection code must handle unnormalized directions
typedef struct Vec3 RayDirection;

// Ray origin point in 3D space
typedef struct Vec3 RayOrigin;

BOOL prepareRaytracing(void);
UWORD fsmRotatingCube(void);
UWORD initRotatingCube(void);
void exitRotatingCube(void);

#endif
