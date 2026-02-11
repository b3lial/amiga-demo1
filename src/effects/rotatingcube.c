#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/exec_protos.h>

#include "rotatingcube.h"

#include "fsmstates.h"
#include "utils/utils.h"
#include "gfx/graphicscontroller.h"
#include "gfx/fixedpoint.h"

struct RotatingCubeContext {
    enum RotatingCubeState state;
    struct BitMap *screenBitmaps[2];
    struct Screen *cubeScreens[2];
    UWORD colorTable[ROTATINGCUBE_SCREEN_COLORS];
    UBYTE currentBufferIndex;  // 0 or 1
    RayDirection *rayDirections;  // Dynamically allocated array of ray directions
    struct Vec3 rayOrigin;  // Shared origin for all rays (camera position)
};

static struct RotatingCubeContext ctx = {
    .state = ROTATINGCUBE_INIT,
    .screenBitmaps = {NULL, NULL},
    .cubeScreens = {NULL, NULL},
    .colorTable = {0},
    .currentBufferIndex = 0,
    .rayDirections = NULL,
    .rayOrigin = {0, 0, 0}  // Will be set to cube object space in generate_screen_rays()
};

//----------------------------------------
// Convert pixel coordinates to normalized device coordinates [-1, 1]
// with aspect ratio correction
static void pixel_to_ndc(UWORD px, UWORD py, UWORD width, UWORD height,
                        WORD *ndc_x, WORD *ndc_y) {
    WORD aspect;

    // Convert to [0, 1] range, then to [-1, 1]
    // Add 0.5 to sample at pixel center
    // Formula: ((px + 0.5) / width) * 2.0 - 1.0

    // For fixed-point: ((px * 256 + 128) * 2 * 256 / width) - 256
    *ndc_x = (WORD)(((((LONG)px << FIXSHIFT) + (1 << (FIXSHIFT - 1))) * (2 << FIXSHIFT)) / width) - (1 << FIXSHIFT);

    // Flip Y axis (screen Y goes down, we want up)
    // Formula: 1.0 - ((py + 0.5) / height) * 2.0
    *ndc_y = (1 << FIXSHIFT) - (WORD)(((((LONG)py << FIXSHIFT) + (1 << (FIXSHIFT - 1))) * (2 << FIXSHIFT)) / height);

    // Apply aspect ratio correction (width/height)
    aspect = safe_fixdiv(INTTOFIX(width), INTTOFIX(height));
    *ndc_x = safe_fixmult(*ndc_x, aspect);
}

//----------------------------------------
// Generate a single ray direction from camera through a pixel
// Note: Direction vectors are NOT normalized - handle this in intersection code
static void generate_ray_direction(UWORD px, UWORD py, UWORD width, UWORD height,
                                   RayDirection *direction) {
    WORD ndc_x, ndc_y;

    // Convert pixel to NDC
    pixel_to_ndc(px, py, width, height, &ndc_x, &ndc_y);

    // Point on screen plane at z = 1.0
    // Direction vector from origin (0,0,0) to point on screen plane
    // NOT normalized - saves 81,920 sqrt operations!
    direction->x = ndc_x;
    direction->y = ndc_y;
    direction->z = SCREEN_PLANE_Z;
}

//----------------------------------------
// Generate ray directions for all screen pixels
// Transforms rays from world space into cube object space
// Returns TRUE on success, FALSE on memory allocation failure
static BOOL generate_screen_rays(UWORD width, UWORD height) {
    ULONG total_rays = (ULONG)width * height;
    ULONG ray_index = 0;
    UWORD px, py;

    writeLogFS("Allocating memory for %lu ray directions (%lu bytes)\n",
               total_rays, total_rays * sizeof(RayDirection));

    // Allocate memory for ray direction array
    ctx.rayDirections = AllocVec(total_rays * sizeof(RayDirection), MEMF_ANY);
    if (!ctx.rayDirections) {
        writeLog("Error: Could not allocate memory for ray directions\n");
        return FALSE;
    }

    writeLog("Generating ray directions for each screen pixel...\n");

    // Generate ray direction for each pixel (in world space)
    for (py = 0; py < height; py++) {
        for (px = 0; px < width; px++) {
            generate_ray_direction(px, py, width, height, &ctx.rayDirections[ray_index]);
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
               ctx.rayOrigin.x, ctx.rayOrigin.y, ctx.rayOrigin.z);
    writeLogFS("Successfully generated %lu ray directions\n", total_rays);

    return TRUE;
}

//----------------------------------------
UWORD fsmRotatingCube(void) {
    if (mouseClick()) {
        ctx.state = ROTATINGCUBE_SHUTDOWN;
    }

    switch (ctx.state) {
        case ROTATINGCUBE_INIT:
            // Generate rays for raytracing
            writeLog("Generating rays for raytracing...\n");
            if (!generate_screen_rays(ROTATINGCUBE_SCREEN_WIDTH, ROTATINGCUBE_SCREEN_HEIGHT)) {
                writeLog("Error: Failed to generate rays\n");
                ctx.state = ROTATINGCUBE_SHUTDOWN;
            } else {
                ctx.state = ROTATINGCUBE_RUNNING;
            }
            break;
        case ROTATINGCUBE_RUNNING:
            // TODO: Implement cube rotation logic
            WaitTOF();
            // Switch buffers
            ctx.currentBufferIndex = 1 - ctx.currentBufferIndex;
            ScreenToFront(ctx.cubeScreens[ctx.currentBufferIndex]);
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

    // Initialize color table (simple grayscale for now)
    for (i = 0; i < ROTATINGCUBE_SCREEN_COLORS; i++) {
        UWORD intensity = (i * 0xf) / (ROTATINGCUBE_SCREEN_COLORS - 1);
        ctx.colorTable[i] = (intensity << 8) | (intensity << 4) | intensity;
    }

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
