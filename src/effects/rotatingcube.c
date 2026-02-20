#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>

#include "rotatingcube.h"

#include "fsmstates.h"
#include "utils/utils.h"
#include "gfx/graphicscontroller.h"
#include "gfx/fixedpoint.h"
#include "gfx/rotation.h"
#include "gfx/chunkyconverter.h"

struct RotatingCubeContext {
    enum RotatingCubeState state;
    struct BitMap *screenBitmaps[2];
    struct Screen *cubeScreens[2];
    UWORD colorTable[ROTATINGCUBE_SCREEN_COLORS];
    UBYTE currentBufferIndex;  // 0 or 1
    RayDirection *rayDirections;  // Dynamically allocated array of ray directions
    struct Vec3 rayOrigin;  // Shared origin for all rays (camera position)
    UBYTE *rotationBuffers[ROTATION_STEPS];  // Chunky buffers for each rotation step
};

static struct RotatingCubeContext ctx = {
    .state = ROTATINGCUBE_INIT,
    .screenBitmaps = {NULL, NULL},
    .cubeScreens = {NULL, NULL},
    .colorTable = {0},
    .currentBufferIndex = 0,
    .rayDirections = NULL,
    .rayOrigin = {0, 0, 0},  // Will be set to cube object space in generate_screen_rays()
    .rotationBuffers = {NULL}  // Will be allocated in initRotatingCube()
};

//----------------------------------------
// Convert pixel coordinates to normalized device coordinates [-1, 1]
// with aspect ratio correction
static void calcPixelNDC(UWORD px, UWORD py, UWORD width, UWORD height,
                        WORD *ndc_x, WORD *ndc_y) {
    WORD aspect;

    // Convert to NDC [-1.0, 1.0] with 0.5 offset to sample pixel center
    // Formula: ((px + 0.5) / width) * 2.0 - 1.0
    *ndc_x = safe_fixmult(safe_fixdiv(INTTOFIX(px) + FLOATTOFIX(0.5),
                                      INTTOFIX(width)),
                          FLOATTOFIX(2.0)) - FLOATTOFIX(1.0);

    // Flip Y axis: screen Y goes down, world Y goes up
    // Formula: 1.0 - ((py + 0.5) / height) * 2.0
    *ndc_y = FLOATTOFIX(1.0) - safe_fixmult(safe_fixdiv(INTTOFIX(py) + FLOATTOFIX(0.5),
                                                         INTTOFIX(height)),
                                             FLOATTOFIX(2.0));

    // Apply aspect ratio correction (width/height)
    aspect = safe_fixdiv(INTTOFIX(width), INTTOFIX(height));
    *ndc_x = safe_fixmult(*ndc_x, aspect);
}

//----------------------------------------
// Generate a single ray direction from camera through a pixel
// Note: Direction vectors are NOT normalized - handle this in intersection code
static void calcRayDirection(UWORD px, UWORD py, UWORD width, UWORD height,
                             RayDirection *direction) {
    WORD ndc_x, ndc_y;

    // Convert pixel to NDC
    calcPixelNDC(px, py, width, height, &ndc_x, &ndc_y);

    // Point on screen plane at z = 1.0
    // Direction vector from origin (0,0,0) to point on screen plane
    // NOT normalized - saves 81,920 sqrt operations!
    direction->x = ndc_x;
    direction->y = ndc_y;
    direction->z = SCREEN_PLANE_Z;
}

//----------------------------------------
// Multiply a 3D vector by an inverse Y-axis rotation matrix (optimized)
// Optimized for Y-axis rotation which has many zeros and ones
// Inverse Y-axis rotation matrix:
// | cos(θ)   0  -sin(θ) |
// |   0      1     0    |
// | sin(θ)   0   cos(θ) |
static void multiplyInverseRotationY(WORD cosVal, WORD sinVal,
                                     const struct Vec3 *vector,
                                     struct Vec3 *result) {
    // result.x = cos(θ)*v.x + 0*v.y - sin(θ)*v.z
    //          = cos(θ)*v.x - sin(θ)*v.z
    result->x = safe_fixmult(cosVal, vector->x) - safe_fixmult(sinVal, vector->z);

    // result.y = 0*v.x + 1*v.y + 0*v.z
    //          = v.y
    result->y = vector->y;

    // result.z = sin(θ)*v.x + 0*v.y + cos(θ)*v.z
    //          = sin(θ)*v.x + cos(θ)*v.z
    result->z = safe_fixmult(sinVal, vector->x) + safe_fixmult(cosVal, vector->z);
}

//----------------------------------------
// Convert a chunky buffer to planar bitmap format
static void convertChunkyToBitmap(UBYTE *sourceChunky, struct BitMap *destPlanar) {
    struct c2pStruct c2p;
    c2p.bmap = destPlanar;
    c2p.startX = 0;
    c2p.startY = 0;
    c2p.width = ROTATINGCUBE_SCREEN_WIDTH;
    c2p.height = ROTATINGCUBE_SCREEN_HEIGHT;
    c2p.chunkybuffer = sourceChunky;
    ChunkyToPlanarAsm(&c2p);
}

//----------------------------------------
// Render all rotation steps of the cube into chunky buffers
// Each step rotates the cube by DEGREE_RESOLUTION degrees
static void renderAllRotationSteps(void) {
    UBYTE step;
    USHORT angle;
    WORD *sinLookup = getSinLookup();
    WORD *cosLookup = getCosLookup();
    ULONG total_rays = (ULONG)ROTATINGCUBE_SCREEN_WIDTH * ROTATINGCUBE_SCREEN_HEIGHT;

    writeLog("Rendering all rotation steps...\n");

    for (step = 0; step < ROTATION_STEPS; step++) {
        WORD sinVal, cosVal;
        struct Vec3 rotatedOrigin;
        RayDirection rotatedDirection;
        ULONG ray_index;

        angle = step * DEGREE_RESOLUTION;
        sinVal = sinLookup[step];
        cosVal = cosLookup[step];

        writeLogFS("Rendering rotation step %d (angle: %d degrees)...\n", step, angle);

        // Transform ray origin with inverse rotation matrix
        multiplyInverseRotationY(cosVal, sinVal, &ctx.rayOrigin, &rotatedOrigin);

        // Transform all ray directions with inverse rotation matrix and raytrace
        for (ray_index = 0; ray_index < total_rays; ray_index++) {
            WORD t_min, t_max, t;
            WORD tx_min, tx_max, ty_min, ty_max, tz_min, tz_max;
            UBYTE color;

            multiplyInverseRotationY(cosVal, sinVal,
                                     &ctx.rayDirections[ray_index],
                                     &rotatedDirection);

            // Ray-AABB intersection using slab method
            // Cube bounds in object space: [-CUBE_HALF_SIZE, CUBE_HALF_SIZE] on each axis
            // t = (bound - origin) / direction

            // X slabs
            if (rotatedDirection.x != 0) {
                tx_min = safe_fixdiv(-CUBE_HALF_SIZE - rotatedOrigin.x, rotatedDirection.x);
                tx_max = safe_fixdiv( CUBE_HALF_SIZE - rotatedOrigin.x, rotatedDirection.x);
                if (tx_min > tx_max) { WORD tmp = tx_min; tx_min = tx_max; tx_max = tmp; }
            } else {
                // Ray parallel to X slabs: check if origin is inside
                tx_min = -32768;
                tx_max =  32767;
            }

            // Y slabs
            if (rotatedDirection.y != 0) {
                ty_min = safe_fixdiv(-CUBE_HALF_SIZE - rotatedOrigin.y, rotatedDirection.y);
                ty_max = safe_fixdiv( CUBE_HALF_SIZE - rotatedOrigin.y, rotatedDirection.y);
                if (ty_min > ty_max) { WORD tmp = ty_min; ty_min = ty_max; ty_max = tmp; }
            } else {
                ty_min = -32768;
                ty_max =  32767;
            }

            // Z slabs
            if (rotatedDirection.z != 0) {
                tz_min = safe_fixdiv(-CUBE_HALF_SIZE - rotatedOrigin.z, rotatedDirection.z);
                tz_max = safe_fixdiv( CUBE_HALF_SIZE - rotatedOrigin.z, rotatedDirection.z);
                if (tz_min > tz_max) { WORD tmp = tz_min; tz_min = tz_max; tz_max = tmp; }
            } else {
                tz_min = -32768;
                tz_max =  32767;
            }

            // Intersect all slabs
            t_min = tx_min > ty_min ? tx_min : ty_min;
            t_min = t_min  > tz_min ? t_min  : tz_min;
            t_max = tx_max < ty_max ? tx_max : ty_max;
            t_max = t_max  < tz_max ? t_max  : tz_max;

            // Hit if t_min <= t_max and intersection is in front of camera
            if (t_min <= t_max && t_max > 0) {
                // t is the entry point (or exit if behind camera)
                t = t_min > 0 ? t_min : t_max;

                // Map distance to color index (closer = brighter)
                // t ranges roughly from 1 to 6 in our scene
                // Map to palette index 1..15 (0 reserved for background)
                color = (UBYTE)(ROTATINGCUBE_SCREEN_COLORS - 1 -
                        (FIXTOINT(t) * (ROTATINGCUBE_SCREEN_COLORS - 2) / 6));
                if (color < 1) color = 1;
                if (color > ROTATINGCUBE_SCREEN_COLORS - 1)
                    color = ROTATINGCUBE_SCREEN_COLORS - 1;
            } else {
                color = 0;  // Background
            }

            ctx.rotationBuffers[step][ray_index] = color;
        }
    }

    writeLogFS("Successfully rendered %d rotation steps\n", ROTATION_STEPS);
}

//----------------------------------------
// Generate ray directions for all screen pixels
// Transforms rays from world space into cube object space
static void calcScreenRays(UWORD width, UWORD height) {
    ULONG ray_index = 0;
    UWORD px, py;

    writeLog("Generating ray directions for each screen pixel...\n");

    // Generate ray direction for each pixel (in world space)
    for (py = 0; py < height; py++) {
        for (px = 0; px < width; px++) {
            calcRayDirection(px, py, width, height, &ctx.rayDirections[ray_index]);
            ray_index++;
        }
    }

    // Transform ray origin from world space to cube object space
    // World space origin: (0, 0, 0)
    // Cube center: (CUBE_CENTER_X, CUBE_CENTER_Y, CUBE_CENTER_Z)
    // Object space origin = world_origin - cube_center
    ctx.rayOrigin.x = 0 - CUBE_CENTER_X;  // 0 - 0 = 0
    ctx.rayOrigin.y = 0 - CUBE_CENTER_Y;  // 0 - 0 = 0
    ctx.rayOrigin.z = 0 - CUBE_CENTER_Z;  // 0 - 3 = -3

    writeLogFS("Ray origin in cube object space: (%d, %d, %d) [fixed-point]\n",
               FIXTOINT(ctx.rayOrigin.x), FIXTOINT(ctx.rayOrigin.y), FIXTOINT(ctx.rayOrigin.z));
    writeLogFS("Successfully generated %lu ray directions\n", (ULONG)width * height);
}

//----------------------------------------
UWORD fsmRotatingCube(void) {
    if (mouseClick()) {
        ctx.state = ROTATINGCUBE_SHUTDOWN;
    }

    switch (ctx.state) {
        case ROTATINGCUBE_INIT:
            // Generate rays for raytracing
            calcScreenRays(ROTATINGCUBE_SCREEN_WIDTH, ROTATINGCUBE_SCREEN_HEIGHT);
            // Render all rotation steps into chunky buffers
            renderAllRotationSteps();
            ctx.state = ROTATINGCUBE_RUNNING;
            break;
        case ROTATINGCUBE_RUNNING:
        {
            static UBYTE stepIndex = 0;
            // Switch buffers
            ctx.currentBufferIndex = 1 - ctx.currentBufferIndex;

            // Convert current chunky buffer to planar bitmap of the back buffer
            convertChunkyToBitmap(ctx.rotationBuffers[stepIndex],
                                  ctx.screenBitmaps[ctx.currentBufferIndex]);

            // Advance to next rotation step (wraps around)
            stepIndex = (stepIndex < ROTATION_STEPS - 1) ? stepIndex + 1 : 0;

            WaitTOF();
            ScreenToFront(ctx.cubeScreens[ctx.currentBufferIndex]);
            ctx.state = ROTATINGCUBE_RUNNING;
            break;
        }
        case ROTATINGCUBE_SHUTDOWN:
            exitRotatingCube();
            return FSM_STOP;
    }

    return FSM_ROTATINGCUBE;
}

//----------------------------------------
UWORD initRotatingCube(void) {
    UBYTE i;
    writeLog("\n\n== initRotatingCube() ==\n");

    // Allocate first screen bitmap
    ctx.screenBitmaps[0] = AllocBitMap(ROTATINGCUBE_SCREEN_WIDTH,
                                       ROTATINGCUBE_SCREEN_HEIGHT,
                                       ROTATINGCUBE_SCREEN_DEPTH,
                                       BMF_DISPLAYABLE | BMF_CLEAR,
                                       NULL);
    if (!ctx.screenBitmaps[0]) {
        writeLog("Error: Could not allocate memory for rotating cube bitmap 0\n");
        goto __exit_init_cube;
    }

    // Allocate second screen bitmap for double buffering
    ctx.screenBitmaps[1] = AllocBitMap(ROTATINGCUBE_SCREEN_WIDTH,
                                       ROTATINGCUBE_SCREEN_HEIGHT,
                                       ROTATINGCUBE_SCREEN_DEPTH,
                                       BMF_DISPLAYABLE | BMF_CLEAR,
                                       NULL);
    if (!ctx.screenBitmaps[1]) {
        writeLog("Error: Could not allocate memory for rotating cube bitmap 1\n");
        goto __exit_init_cube;
    }

    // Initialize color table (simple grayscale for now)
    for (i = 0; i < ROTATINGCUBE_SCREEN_COLORS; i++) {
        UWORD intensity = (i * 0xf) / (ROTATINGCUBE_SCREEN_COLORS - 1);
        ctx.colorTable[i] = (intensity << 8) | (intensity << 4) | intensity;
    }

    // Allocate memory for ray direction array
    {
        ULONG total_rays = (ULONG)ROTATINGCUBE_SCREEN_WIDTH * ROTATINGCUBE_SCREEN_HEIGHT;
        writeLogFS("Allocating memory for %lu ray directions (%lu bytes)\n",
                   total_rays, total_rays * sizeof(RayDirection));
        ctx.rayDirections = AllocVec(total_rays * sizeof(RayDirection), MEMF_ANY);
        if (!ctx.rayDirections) {
            writeLog("Error: Could not allocate memory for ray directions\n");
            goto __exit_init_cube;
        }
    }

    // Allocate chunky buffers for each rotation step
    writeLogFS("Allocating %d chunky buffers for rotation steps...\n", ROTATION_STEPS);
    for (i = 0; i < ROTATION_STEPS; i++) {
        ULONG bufferSize = (ULONG)ROTATINGCUBE_SCREEN_WIDTH * ROTATINGCUBE_SCREEN_HEIGHT;
        ctx.rotationBuffers[i] = AllocVec(bufferSize, MEMF_FAST | MEMF_CLEAR);
        if (!ctx.rotationBuffers[i]) {
            writeLogFS("Error: Could not allocate chunky buffer %d (%lu bytes)\n", i, bufferSize);
            goto __exit_init_cube;
        }
    }
    writeLogFS("Successfully allocated %d chunky buffers (%lu bytes each)\n",
               ROTATION_STEPS, (ULONG)ROTATINGCUBE_SCREEN_WIDTH * ROTATINGCUBE_SCREEN_HEIGHT);

    // Create first screen
    ctx.cubeScreens[0] = createScreen(ctx.screenBitmaps[0], TRUE,
                                      0, 0,
                                      ROTATINGCUBE_SCREEN_WIDTH,
                                      ROTATINGCUBE_SCREEN_HEIGHT,
                                      ROTATINGCUBE_SCREEN_DEPTH, NULL);
    if (!ctx.cubeScreens[0]) {
        writeLog("Error: Could not allocate memory for rotating cube screen 0\n");
        goto __exit_init_cube;
    }

    // Create second screen for double buffering
    ctx.cubeScreens[1] = createScreen(ctx.screenBitmaps[1], TRUE,
                                      0, 0,
                                      ROTATINGCUBE_SCREEN_WIDTH,
                                      ROTATINGCUBE_SCREEN_HEIGHT,
                                      ROTATINGCUBE_SCREEN_DEPTH, NULL);
    if (!ctx.cubeScreens[1]) {
        writeLog("Error: Could not allocate memory for rotating cube screen 1\n");
        goto __exit_init_cube;
    }

    // Init double buffering - start with buffer 0
    ctx.currentBufferIndex = 0;

    // Load color tables for both screens
    LoadRGB4(&ctx.cubeScreens[0]->ViewPort, ctx.colorTable, ROTATINGCUBE_SCREEN_COLORS);
    LoadRGB4(&ctx.cubeScreens[1]->ViewPort, ctx.colorTable, ROTATINGCUBE_SCREEN_COLORS);

    // Make screen visible
    ScreenToFront(ctx.cubeScreens[ctx.currentBufferIndex]);
    return FSM_ROTATINGCUBE;

__exit_init_cube:
    exitRotatingCube();
    return FSM_ERROR;
}

//----------------------------------------
void exitRotatingCube(void) {
    UBYTE i;
    writeLog("\n== exitRotatingCube() ==\n");

    // Free chunky rotation buffers
    for (i = 0; i < ROTATION_STEPS; i++) {
        if (ctx.rotationBuffers[i]) {
            FreeVec(ctx.rotationBuffers[i]);
            ctx.rotationBuffers[i] = NULL;
        }
    }

    // Free ray direction array
    if (ctx.rayDirections) {
        writeLog("Freeing ray direction array\n");
        FreeVec(ctx.rayDirections);
        ctx.rayDirections = NULL;
    }

    WaitTOF();
    for (i = 0; i < 2; i++) {
        if (ctx.cubeScreens[i]) {
            CloseScreen(ctx.cubeScreens[i]);
            ctx.cubeScreens[i] = NULL;
        }
    }
    WaitTOF();

    for (i = 0; i < 2; i++) {
        if (ctx.screenBitmaps[i]) {
            FreeBitMap(ctx.screenBitmaps[i]);
            ctx.screenBitmaps[i] = NULL;
        }
    }

    ctx.state = ROTATINGCUBE_INIT;
}
