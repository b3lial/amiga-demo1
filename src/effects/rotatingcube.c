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
    struct Ray *rays;  // Dynamically allocated ray array
};

static struct RotatingCubeContext ctx = {
    .state = ROTATINGCUBE_INIT,
    .screenBitmaps = {NULL, NULL},
    .cubeScreens = {NULL, NULL},
    .colorTable = {0},
    .currentBufferIndex = 0,
    .rays = NULL
};

//----------------------------------------
// Fast inverse square root using integer approximation
// Adapted from Quake III for fixed-point arithmetic
// Returns 1/sqrt(x) in fixed-point format
static WORD fast_inv_sqrt_fix(LONG x) {
    LONG i;
    LONG x2, y;
    const LONG threehalfs = FLOATTOFIX(1.5);

    // Prevent division by zero or negative values
    if (x <= 0) {
        return FLOATTOFIX(1.0);
    }

    x2 = x >> 1;  // x2 = x / 2

    // Initial guess using bit manipulation
    // This is a simplified version for 32-bit integers
    i = x;
    i = 0x5f3759df - (i >> 1);  // Magic number from Quake III
    y = i;

    // Newton-Raphson iteration: y = y * (1.5 - (x2 * y * y))
    // One iteration is usually enough for our precision needs
    {
        LONG y_squared = (y * y) >> FIXSHIFT;
        LONG product = (x2 * y_squared) >> FIXSHIFT;
        LONG factor = threehalfs - product;
        y = (y * factor) >> FIXSHIFT;
    }

    return (WORD)y;
}

//----------------------------------------
// Normalize a 3D vector to unit length using fast inverse sqrt
static void normalize_vec3(struct Vec3 *v) {
    LONG len_squared;
    WORD inv_len;

    // Calculate length squared (x^2 + y^2 + z^2)
    len_squared = ((LONG)v->x * v->x) + ((LONG)v->y * v->y) + ((LONG)v->z * v->z);

    // Skip normalization if vector is too small
    if (len_squared < 16) {  // Threshold to avoid division issues
        v->x = 0;
        v->y = 0;
        v->z = FLOATTOFIX(1.0);
        return;
    }

    // Get inverse length using fast approximation
    inv_len = fast_inv_sqrt_fix(len_squared);

    // Multiply each component by inverse length
    v->x = safe_fixmult(v->x, inv_len);
    v->y = safe_fixmult(v->y, inv_len);
    v->z = safe_fixmult(v->z, inv_len);
}

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
// Generate a single ray from camera through a pixel
static void generate_ray(UWORD px, UWORD py, UWORD width, UWORD height,
                        struct Ray *ray) {
    WORD ndc_x, ndc_y;

    // Camera origin at (0, 0, 0)
    ray->origin.x = 0;
    ray->origin.y = 0;
    ray->origin.z = 0;

    // Convert pixel to NDC
    pixel_to_ndc(px, py, width, height, &ndc_x, &ndc_y);

    // Point on screen plane at z = 1.0
    ray->direction.x = ndc_x;
    ray->direction.y = ndc_y;
    ray->direction.z = SCREEN_PLANE_Z;

    // Normalize direction vector
    normalize_vec3(&ray->direction);
}

//----------------------------------------
// Generate rays for all screen pixels
// Returns TRUE on success, FALSE on memory allocation failure
static BOOL generate_screen_rays(UWORD width, UWORD height) {
    ULONG total_rays = (ULONG)width * height;
    ULONG ray_index = 0;
    UWORD px, py;

    writeLogFS("Allocating memory for %lu rays (%lu bytes)\n",
               total_rays, total_rays * sizeof(struct Ray));

    // Allocate memory for ray array
    ctx.rays = AllocVec(total_rays * sizeof(struct Ray), MEMF_ANY);
    if (!ctx.rays) {
        writeLog("Error: Could not allocate memory for ray array\n");
        return FALSE;
    }

    writeLog("Generating rays for each screen pixel...\n");

    // Generate ray for each pixel
    for (py = 0; py < height; py++) {
        for (px = 0; px < width; px++) {
            generate_ray(px, py, width, height, &ctx.rays[ray_index]);
            ray_index++;
        }
    }

    writeLogFS("Successfully generated %lu rays\n", total_rays);
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

    // Free ray array
    if (ctx.rays) {
        writeLog("Freeing ray array\n");
        FreeVec(ctx.rays);
        ctx.rays = NULL;
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
