#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h>

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
    ULONG colorTable[ROTATINGCUBE_SCREEN_COLORS * 3 + 2];  // LoadRGB32 format: (count<<16)|0, R, G, B, ..., 0
    UBYTE currentBufferIndex;  // 0 or 1
    RayDirection *rayDirections;  // Dynamically allocated array of ray directions
    RayOrigin rayOrigin;  // Shared origin for all rays (camera position)
    UBYTE *rotationBuffers[ROTATION_STEPS];    // Chunky buffers for each rotation step
    UBYTE *silhouetteBuffers[ROTATION_STEPS];  // Silhouette masks for cookie cut blit (1=cube, 0=background)
    struct BitMap *cubePlanarBitmap;           // Intermediate planar buffer for rendered cube
    struct BitMap *silhouettePlanarBitmap;     // Intermediate planar buffer for silhouette mask (1 bitplane)
    struct Task *mainTask;                     // Main task pointer for signaling
    struct Task *bgTask;                       // Background raytracing preparation task
    BOOL raytracingStarted;                    // TRUE if prepareRaytracing() has already been called
};

static struct RotatingCubeContext ctx = {
    .state = ROTATINGCUBE_INIT,
    .screenBitmaps = {NULL, NULL},
    .cubeScreens = {NULL, NULL},
    .colorTable = {0},
    .currentBufferIndex = 0,
    .rayDirections = NULL,
    .rayOrigin = {0, 0, 0},  // Will be set to cube object space in generate_screen_rays()
    .rotationBuffers = {NULL},      // Will be allocated in initRotatingCube()
    .silhouetteBuffers = {NULL},    // Will be allocated in initRotatingCube()
    .cubePlanarBitmap = NULL,
    .silhouettePlanarBitmap = NULL,
    .mainTask = NULL,
    .bgTask = NULL,
    .raytracingStarted = FALSE
};

//----------------------------------------
// Convert pixel coordinates to normalized device coordinates [-1, 1]
// with aspect ratio correction
// Uses LONG (32-bit) 24.8 fixed-point internally for precision, then converts to WORD 8.8
static void calcPixelNDC(UWORD px, UWORD py, UWORD width, UWORD height,
                        fix16 *ndc_x, fix16 *ndc_y) {
    fix32 temp_x, temp_y;
    fix32 aspect;

    // Convert to NDC [-1.0, 1.0] with 0.5 offset to sample pixel center
    // Formula: ((px + 0.5) / width) * 2.0 - 1.0
    // Use LONG 24.8 format for full precision
    temp_x = INTTOFIX_LONG(px) + FLOATTOFIX_LONG(0.5);
    temp_x = FIXDIV_LONG(temp_x, INTTOFIX_LONG(width));
    temp_x = FIXMULT_LONG(temp_x, FLOATTOFIX_LONG(2.0));
    temp_x = temp_x - FLOATTOFIX_LONG(1.0);

    // Flip Y axis: screen Y goes down, world Y goes up
    // Formula: 1.0 - ((py + 0.5) / height) * 2.0
    temp_y = INTTOFIX_LONG(py) + FLOATTOFIX_LONG(0.5);
    temp_y = FIXDIV_LONG(temp_y, INTTOFIX_LONG(height));
    temp_y = FIXMULT_LONG(temp_y, FLOATTOFIX_LONG(2.0));
    temp_y = FLOATTOFIX_LONG(1.0) - temp_y;

    // Apply pixel aspect ratio correction for Amiga 320x256 PAL
    // Use PIXEL_ASPECT_RATIO defined in header - adjust value there to get square cube
    aspect = FIXTOLONG(PIXEL_ASPECT_RATIO);
    temp_x = FIXMULT_LONG(temp_x, aspect);

    // Convert from LONG 24.8 to WORD 8.8 with clamping
    if (temp_x > WORD_MAX) temp_x = WORD_MAX;
    if (temp_x < WORD_MIN) temp_x = WORD_MIN;
    if (temp_y > WORD_MAX) temp_y = WORD_MAX;
    if (temp_y < WORD_MIN) temp_y = WORD_MIN;

    *ndc_x = LONGTOFIX(temp_x);
    *ndc_y = LONGTOFIX(temp_y);
}

//----------------------------------------
// Generate a single ray direction from camera through a pixel
// Note: Direction vectors are NOT normalized - handle this in intersection code
static void calcRayDirection(UWORD px, UWORD py, UWORD width, UWORD height,
                             RayDirection *direction) {
    fix16 ndc_x, ndc_y;

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
// Multiply a 3D vector by combined inverse X, Y, and Z rotation matrices
// Applies inverse Z, then Y, then X-axis rotations
// Combined matrix = Rx^-1 * Ry^-1 * Rz^-1
//
// Standard forward rotations:
// Rx = | 1    0      0   |    Ry = | cos  0  sin |    Rz = | cos -sin  0 |
//      | 0  cos  -sin    |         |  0   1   0  |         | sin  cos  0 |
//      | 0  sin   cos    |         |-sin  0  cos |         |  0    0   1 |
//
// Inverse rotations (transpose):
// Rx^-1 = | 1   0     0  |   Ry^-1 = | cos  0 -sin |   Rz^-1 = | cos  sin  0 |
//         | 0  cos  sin  |           |  0   1   0  |           |-sin  cos  0 |
//         | 0 -sin  cos  |           | sin  0  cos |           |  0    0   1 |
static void multiplyInverseRotationXYZ(fix16 cosX, fix16 sinX, fix16 cosY, fix16 sinY, fix16 cosZ, fix16 sinZ,
                                       const struct Vec3 *vector,
                                       struct Vec3 *result) {
    struct Vec3 temp1, temp2;

    // First apply inverse Z-axis rotation: Rz^-1
    temp1.x = safe_fixmult(cosZ, vector->x) + safe_fixmult(sinZ, vector->y);
    temp1.y = -safe_fixmult(sinZ, vector->x) + safe_fixmult(cosZ, vector->y);
    temp1.z = vector->z;

    // Then apply inverse Y-axis rotation: Ry^-1
    temp2.x = safe_fixmult(cosY, temp1.x) + safe_fixmult(sinY, temp1.z);
    temp2.y = temp1.y;
    temp2.z = -safe_fixmult(sinY, temp1.x) + safe_fixmult(cosY, temp1.z);

    // Finally apply inverse X-axis rotation: Rx^-1
    result->x = temp2.x;
    result->y = safe_fixmult(cosX, temp2.y) + safe_fixmult(sinX, temp2.z);
    result->z = -safe_fixmult(sinX, temp2.y) + safe_fixmult(cosX, temp2.z);
}

//----------------------------------------
// Convert a chunky buffer to planar bitmap format
static void convertChunkyToBitmap(UBYTE *sourceChunky, struct BitMap *destPlanar) {
    struct c2pStruct c2p;
    c2p.bmap = destPlanar;
    c2p.startX = 0;
    c2p.startY = 0;
    c2p.width = CUBE_INNER_WIDTH;
    c2p.height = CUBE_INNER_HEIGHT;
    c2p.chunkybuffer = sourceChunky;
    ChunkyToPlanarAsm(&c2p);
}

//----------------------------------------
// Calculate pixel color based on distance to cube
UBYTE calculateColor(fix16 t_min, fix16 t_max)
{
    UBYTE color = 0;
    fix16 t;
    fix16 distanceDiff;

    // Hit if t_min <= t_max and intersection is in front of camera
    if (t_min <= t_max && t_max > 0) {
        // t is the entry point (or exit if behind camera)
        t = t_min > 0 ? t_min : t_max;

        distanceDiff = t - CUBE_MIN_DISTANCE;
        color = (UBYTE) FIXTOINT(safe_fixdiv(distanceDiff, CUBE_COLOR_SECTION_SIZE));
        color = (CUBE_COLORS - 1) - color;
        color += 2;  // skip index 0 (background) and index 1 (reserved)
        color = ((color >= ROTATINGCUBE_SCREEN_COLORS) ? 0 : color);
    } 
    else 
    {
        color = 0;  // Background
    }

    return color;
} 

//----------------------------------------
// Ray-AABB intersection using slab method
// Cube bounds in object space: [-CUBE_HALF_SIZE, CUBE_HALF_SIZE] on each axis
void rayIntersectionWithSlab(RayOrigin *rotatedOrigin, RayDirection *rotatedDirection, fix16 *t_min, fix16 *t_max)
{
    fix16 tx_min, tx_max, ty_min, ty_max, tz_min, tz_max;

    // X slabs
    if (rotatedDirection->x != 0) {
        tx_min = safe_fixdiv(-CUBE_HALF_SIZE - rotatedOrigin->x, rotatedDirection->x);
        tx_max = safe_fixdiv( CUBE_HALF_SIZE - rotatedOrigin->x, rotatedDirection->x);
        if (tx_min > tx_max) { WORD tmp = tx_min; tx_min = tx_max; tx_max = tmp; }
    } else {
        // Ray parallel to X slabs: check if origin is inside
        if (rotatedOrigin->x < -CUBE_HALF_SIZE || rotatedOrigin->x > CUBE_HALF_SIZE) {
            // Origin outside cube bounds - no intersection possible
            tx_min = INTTOFIX(1);
            tx_max = INTTOFIX(0);  // tx_min > tx_max will cause miss
        } else {
            // Origin inside - ray passes through entire range
            tx_min = FIX_MIN_INFINITY;
            tx_max = FIX_MAX_INFINITY;
        }
    }

    // Y slabs
    if (rotatedDirection->y != 0) {
        ty_min = safe_fixdiv(-CUBE_HALF_SIZE - rotatedOrigin->y, rotatedDirection->y);
        ty_max = safe_fixdiv( CUBE_HALF_SIZE - rotatedOrigin->y, rotatedDirection->y);
        if (ty_min > ty_max) { WORD tmp = ty_min; ty_min = ty_max; ty_max = tmp; }
    } else {
        if (rotatedOrigin->y < -CUBE_HALF_SIZE || rotatedOrigin->y > CUBE_HALF_SIZE) {
            ty_min = INTTOFIX(1);
            ty_max = INTTOFIX(0);
        } else {
            ty_min = FIX_MIN_INFINITY;
            ty_max = FIX_MAX_INFINITY;
        }
    }

    // Z slabs
    if (rotatedDirection->z != 0) {
        tz_min = safe_fixdiv(-CUBE_HALF_SIZE - rotatedOrigin->z, rotatedDirection->z);
        tz_max = safe_fixdiv( CUBE_HALF_SIZE - rotatedOrigin->z, rotatedDirection->z);
        if (tz_min > tz_max) { WORD tmp = tz_min; tz_min = tz_max; tz_max = tmp; }
    } else {
        if (rotatedOrigin->z < -CUBE_HALF_SIZE || rotatedOrigin->z > CUBE_HALF_SIZE) {
            tz_min = INTTOFIX(1);
            tz_max = INTTOFIX(0);
        } else {
            tz_min = FIX_MIN_INFINITY;
            tz_max = FIX_MAX_INFINITY;
        }
    }

    // Intersect all slabs
    *t_min = tx_min > ty_min ? tx_min : ty_min;
    *t_min = *t_min  > tz_min ? *t_min  : tz_min;
    *t_max = tx_max < ty_max ? tx_max : ty_max;
    *t_max = *t_max  < tz_max ? *t_max  : tz_max;
}

//----------------------------------------
// Render all rotation steps of the cube into chunky buffers
// Each step rotates the cube by DEGREE_RESOLUTION degrees
static void renderAllRotationSteps(void) {
    UBYTE step;
    fix16 *sinLookup = getSinLookup();
    fix16 *cosLookup = getCosLookup();
    ULONG total_rays = (ULONG)CUBE_INNER_WIDTH * CUBE_INNER_HEIGHT;

    for (step = 0; step < ROTATION_STEPS; step++) {
        fix16 sinValX, cosValX, sinValY, cosValY, sinValZ, cosValZ;
        RayOrigin rotatedOrigin;
        RayDirection rotatedDirection;
        ULONG ray_index;

        // X and Z axis rotation for testing
        sinValX = sinLookup[step];
        cosValX = cosLookup[step];
        sinValY = FLOATTOFIX(0.0);  // No Y rotation
        cosValY = FLOATTOFIX(1.0);
        sinValZ = sinLookup[step];
        cosValZ = cosLookup[step];

        // Transform ray origin with inverse rotation matrix (X and Z axes)
        multiplyInverseRotationXYZ(cosValX, sinValX, cosValY, sinValY, cosValZ, sinValZ, &ctx.rayOrigin, &rotatedOrigin);

        // Transform all ray directions with inverse rotation matrix and raytrace
        for (ray_index = 0; ray_index < total_rays; ray_index++) {
            fix16 t_min, t_max;

            multiplyInverseRotationXYZ(cosValX, sinValX, cosValY, sinValY, cosValZ, sinValZ,
                                       &ctx.rayDirections[ray_index],
                                       &rotatedDirection);

            // Ray-AABB intersection using slab method
            rayIntersectionWithSlab(&rotatedOrigin, &rotatedDirection, 
                &t_min, &t_max
            );

            // calculate pixel color based on intersection distance
            ctx.rotationBuffers[step][ray_index] = calculateColor(t_min, t_max);

            // silhouette: 1 where cube is present, 0 for background
            ctx.silhouetteBuffers[step][ray_index] = (ctx.rotationBuffers[step][ray_index] != 0) ? 1 : 0;
        }
    }
}

//----------------------------------------
// Generate ray directions for all screen pixels
// Transforms rays from world space into cube object space
static void calcScreenRays(UWORD width, UWORD height) {
    ULONG ray_index = 0;
    UWORD px, py;

    // Generate ray directions for the inner region only.
    // NDC is calculated against the full screen dims to preserve correct projection.
    // Pixel (BORDER_WIDTH + x, BORDER_HEIGHT + y) is stored at inner index (y * CUBE_INNER_WIDTH + x).
    for (py = BORDER_HEIGHT; py < height - BORDER_HEIGHT; py++) {
        for (px = BORDER_WIDTH; px < width - BORDER_WIDTH; px++) {
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
}

static void prepareRaytracingTask(void);  // forward declaration

//----------------------------------------
BOOL prepareRaytracing(void) {
    UBYTE i;

    // Allocate memory for ray direction array
    {
        ULONG total_rays = (ULONG)CUBE_INNER_WIDTH * CUBE_INNER_HEIGHT;
        ctx.rayDirections = AllocVec(total_rays * sizeof(RayDirection), MEMF_ANY);
        if (!ctx.rayDirections) {
            return FALSE;
        }
    }

    // Allocate chunky buffers for each rotation step
    for (i = 0; i < ROTATION_STEPS; i++) {
        ULONG bufferSize = (ULONG)CUBE_INNER_WIDTH * CUBE_INNER_HEIGHT;
        ctx.rotationBuffers[i] = AllocVec(bufferSize, MEMF_FAST | MEMF_CLEAR);
        if (!ctx.rotationBuffers[i]) {
            return FALSE;
        }
    }

    // Allocate silhouette chunky buffers (one per rotation step)
    for (i = 0; i < ROTATION_STEPS; i++) {
        ULONG bufferSize = (ULONG)CUBE_INNER_WIDTH * CUBE_INNER_HEIGHT;
        ctx.silhouetteBuffers[i] = AllocVec(bufferSize, MEMF_FAST | MEMF_CLEAR);
        if (!ctx.silhouetteBuffers[i]) {
            return FALSE;
        }
    }

    // Allocate intermediate planar bitmap for the rendered cube
    ctx.cubePlanarBitmap = AllocBitMap(CUBE_INNER_WIDTH, CUBE_INNER_HEIGHT,
                                       ROTATINGCUBE_SCREEN_DEPTH, BMF_CLEAR, NULL);
    if (!ctx.cubePlanarBitmap) {
        return FALSE;
    }

    // Allocate intermediate planar bitmap for the silhouette mask (1 bitplane)
    ctx.silhouettePlanarBitmap = AllocBitMap(CUBE_INNER_WIDTH, CUBE_INNER_HEIGHT,
                                             1, BMF_CLEAR, NULL);
    if (!ctx.silhouettePlanarBitmap) {
        return FALSE;
    }

    // Start background raytracing task
    ctx.mainTask = FindTask(NULL);
    SetSignal(0, SIGF_CUBE_PREPARE_DONE);
    ctx.bgTask = (struct Task *)CreateTask(
        (CONST_STRPTR)"PrepareRaytracing", 0,
        (APTR)prepareRaytracingTask, 4096);
    if (!ctx.bgTask) {
        return FALSE;
    }

    ctx.raytracingStarted = TRUE;
    return TRUE;
}

//----------------------------------------
static void prepareRaytracingTask(void) {
    // Lower priority so main task keeps running smoothly
    SetTaskPri(FindTask(NULL), -5);

    calcScreenRays(ROTATINGCUBE_SCREEN_WIDTH, ROTATINGCUBE_SCREEN_HEIGHT);
    renderAllRotationSteps();

    Signal(ctx.mainTask, SIGF_CUBE_PREPARE_DONE);
}

// Four diagonal directions: (dirX, dirY) pairs
// 0=bottom-right, 1=bottom-left, 2=top-right, 3=top-left
static const WORD gridDirX[4] = { 1, -1,  1, -1 };
static const WORD gridDirY[4] = { 1,  1, -1, -1 };

//----------------------------------------
static void drawGrid(struct RastPort *rp) {
    static UWORD gridOffsetX = 0;
    static UWORD gridOffsetY = 0;
    static UWORD directionCounter = 0;
    static UBYTE currentDir = 0;
    WORD x, y;

    directionCounter++;
    if (directionCounter >= GRID_DIRECTION_FRAMES) {
        directionCounter = 0;
        currentDir = (UBYTE) RangeRand(4);
    }

    gridOffsetX = (gridOffsetX + GRID_SPACING + gridDirX[currentDir]) % GRID_SPACING;
    gridOffsetY = (gridOffsetY + GRID_SPACING + gridDirY[currentDir]) % GRID_SPACING;

    SetAPen(rp, 1);  // palette index 1 = white

    // Horizontal lines
    for (y = (WORD)gridOffsetY - GRID_SPACING; y < ROTATINGCUBE_SCREEN_HEIGHT; y += GRID_SPACING) {
        if (y < 0) continue;
        Move(rp, 0, y);
        Draw(rp, ROTATINGCUBE_SCREEN_WIDTH - 1, y);
    }

    // Vertical lines
    for (x = (WORD)gridOffsetX - GRID_SPACING; x < ROTATINGCUBE_SCREEN_WIDTH; x += GRID_SPACING) {
        if (x < 0) continue;
        Move(rp, x, 0);
        Draw(rp, x, ROTATINGCUBE_SCREEN_HEIGHT - 1);
    }
}

//----------------------------------------
static void draw(void) {
    static UBYTE stepIndex = 0;

    // Switch buffers
    ctx.currentBufferIndex = 1 - ctx.currentBufferIndex;

    // Convert chunky buffers to intermediate planar bitmaps
    convertChunkyToBitmap(ctx.rotationBuffers[stepIndex],   ctx.cubePlanarBitmap);
    convertChunkyToBitmap(ctx.silhouetteBuffers[stepIndex], ctx.silhouettePlanarBitmap);

    // Clear back screen buffer to background color before compositing
    SetRast(&ctx.cubeScreens[ctx.currentBufferIndex]->RastPort, 0);

    // Draw white grid into the back screen buffer
    drawGrid(&ctx.cubeScreens[ctx.currentBufferIndex]->RastPort);

    /*
     * Cookie-cut blit: composite cube over grid background using silhouette mask.
     * BltMaskBitMapRastPort uses channel assignment A=Source, B=Mask, C=Dest
     * (unlike direct hardware blitter where A=Mask, B=Source, C=Dest).
     * Minterm 0xE2 = AB + !BC: where mask=1 take source, where mask=0 keep dest.
     */
    BltMaskBitMapRastPort(ctx.cubePlanarBitmap, 0, 0,
                          &ctx.cubeScreens[ctx.currentBufferIndex]->RastPort,
                          BORDER_WIDTH, BORDER_HEIGHT,
                          CUBE_INNER_WIDTH, CUBE_INNER_HEIGHT,
                          0xE2,
                          ctx.silhouettePlanarBitmap->Planes[0]);
    WaitBlit();

    ScreenToFront(ctx.cubeScreens[ctx.currentBufferIndex]);

    stepIndex = (stepIndex < ROTATION_STEPS - 1) ? stepIndex + 1 : 0;
}

//----------------------------------------
UWORD fsmRotatingCube(void) {
    if (mouseClick()) {
        ctx.state = ROTATINGCUBE_SHUTDOWN;
    }

    switch (ctx.state) {
        case ROTATINGCUBE_INIT:
            // If prepareRaytracing() was already called externally (e.g. from showlogo),
            // skip straight to waiting for the signal
            ctx.state = ctx.raytracingStarted ? ROTATINGCUBE_WAIT_PREPARE : ROTATINGCUBE_PREPARE;
            break;
        case ROTATINGCUBE_PREPARE:
            if (!prepareRaytracing())
            {
                writeLog("Error: Could not prepare raytracing\n");
                ctx.state = ROTATINGCUBE_SHUTDOWN;
            } 
            else
            {
                ctx.state = ROTATINGCUBE_WAIT_PREPARE;
            }
            break;
        case ROTATINGCUBE_WAIT_PREPARE: 
        {
            ULONG receivedSignals;
            // Yield CPU so the lower-priority background task can run
            WaitTOF();
            receivedSignals = SetSignal(0, 0);
            if (receivedSignals & SIGF_CUBE_PREPARE_DONE) {
                ctx.bgTask = NULL;
                ctx.state = ROTATINGCUBE_RUNNING;
            }
            break;
        }
        case ROTATINGCUBE_RUNNING:
            draw();
            break;
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

    // Initialize color table for LoadRGB32 (AGA 8-bit per channel)
    // Format: (count<<16)|first_register, R, G, B, R, G, B, ..., 0
    // Each RGB component is 32-bit: 0xRRRRRRRR (8-bit value repeated 4 times)
    // Medium blue palette: varying blue intensity with some green, minimal red
    ctx.colorTable[0] = (ROTATINGCUBE_SCREEN_COLORS << 16) | 0;  // Load 32 colors starting at register 0

    // Index 0: background (black)
    ctx.colorTable[1 + 0 * 3 + 0] = 0x00000000;
    ctx.colorTable[1 + 0 * 3 + 1] = 0x00000000;
    ctx.colorTable[1 + 0 * 3 + 2] = 0x00000000;

    // Index 1: reserved color (white)
    ctx.colorTable[1 + 1 * 3 + 0] = 0xFFFFFFFF;
    ctx.colorTable[1 + 1 * 3 + 1] = 0xFFFFFFFF;
    ctx.colorTable[1 + 1 * 3 + 2] = 0xFFFFFFFF;

    // Indices 2..31: cube depth gradient (CUBE_COLORS = 30 steps)
    for (i = 2; i < ROTATINGCUBE_SCREEN_COLORS; i++) {
        ULONG intensity = ((i - 2) * 0xFF) / (CUBE_COLORS - 1);  // 0-255 range across 30 steps
        ULONG red = intensity / 8;           // Very little red (1/8 intensity)
        ULONG green = intensity / 2;         // Medium green (1/2 intensity)
        ULONG blue = intensity;              // Full blue intensity
        // Replicate 8-bit value across all 4 bytes: 0xRRRRRRRR
        ctx.colorTable[1 + i * 3 + 0] = (red << 24) | (red << 16) | (red << 8) | red;
        ctx.colorTable[1 + i * 3 + 1] = (green << 24) | (green << 16) | (green << 8) | green;
        ctx.colorTable[1 + i * 3 + 2] = (blue << 24) | (blue << 16) | (blue << 8) | blue;
    }
    ctx.colorTable[1 + ROTATINGCUBE_SCREEN_COLORS * 3] = 0;  // Terminator

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

    // Load color tables for both screens using AGA LoadRGB32
    LoadRGB32(&ctx.cubeScreens[0]->ViewPort, ctx.colorTable);
    LoadRGB32(&ctx.cubeScreens[1]->ViewPort, ctx.colorTable);

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

    // Wait for background task to finish if still running
    if (ctx.bgTask) {
        ULONG receivedSignals = SetSignal(0, 0);
        if (!(receivedSignals & SIGF_CUBE_PREPARE_DONE)) {
            writeLog("Waiting for raytracing background task to complete...\n");
            Wait(SIGF_CUBE_PREPARE_DONE);
        }
        SetSignal(0, SIGF_CUBE_PREPARE_DONE);
        ctx.bgTask = NULL;
    }
    writeLog("\n== exitRotatingCube() ==\n");

    // Free chunky rotation buffers
    for (i = 0; i < ROTATION_STEPS; i++) {
        if (ctx.rotationBuffers[i]) {
            FreeVec(ctx.rotationBuffers[i]);
            ctx.rotationBuffers[i] = NULL;
        }
    }

    // Free silhouette buffers
    for (i = 0; i < ROTATION_STEPS; i++) {
        if (ctx.silhouetteBuffers[i]) {
            FreeVec(ctx.silhouetteBuffers[i]);
            ctx.silhouetteBuffers[i] = NULL;
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

    if (ctx.cubePlanarBitmap) {
        FreeBitMap(ctx.cubePlanarBitmap);
        ctx.cubePlanarBitmap = NULL;
    }

    if (ctx.silhouettePlanarBitmap) {
        FreeBitMap(ctx.silhouettePlanarBitmap);
        ctx.silhouettePlanarBitmap = NULL;
    }

    for (i = 0; i < 2; i++) {
        if (ctx.screenBitmaps[i]) {
            FreeBitMap(ctx.screenBitmaps[i]);
            ctx.screenBitmaps[i] = NULL;
        }
    }

    ctx.state = ROTATINGCUBE_INIT;
    ctx.raytracingStarted = FALSE;
}
