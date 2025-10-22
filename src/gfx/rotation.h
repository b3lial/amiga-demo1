#ifndef ROTATION_H
#define ROTATION_H

#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/rastport.h>
#include <graphics/videocontrol.h>

// #define NATIVE_CONVERTER

// credits to: https://coronax.wordpress.com/2014/01/31/running-with-the-numbers/
#define FIXSHIFT 8
// convert float to fix (and back)
#define FLOATTOFIX(x) ((WORD)((x) * (1 << FIXSHIFT)))
#define FIXTOFLOAT(x) ((float)(x) / (1 << FIXSHIFT))
// convert int to fix (and back)
#define INTTOFIX(x) ((x) << FIXSHIFT)
#define FIXTOINT(x) ((x) >> FIXSHIFT)
// multiply and divide (unsafe - use safe_fixmult/safe_fixdiv instead)
#define FIXMULT(x, y) (((x) * (y)) >> FIXSHIFT)
#define FIXDIV(x, y) (((x) << FIXSHIFT) / (y))

// Safe fixed-point operations with overflow protection
static inline WORD safe_fixmult(WORD x, WORD y) {
    LONG result = ((LONG)x * (LONG)y) >> FIXSHIFT;
    if (result > 32767) return 32767;
    if (result < -32768) return -32768;
    return (WORD)result;
}

static inline WORD safe_fixdiv(WORD x, WORD y) {
    if (y == 0) return 0;  // Division by zero protection
    LONG result = ((LONG)x << FIXSHIFT) / (LONG)y;
    if (result > 32767) return 32767;
    if (result < -32768) return -32768;
    return (WORD)result;
}

#define DEGREE_RESOLUTION 10
#define DEST_BUFFER_SIZE 36

#define MAX_BITMAP_WIDTH 320
#define MAX_BITMAP_HEIGHT 256

BOOL startRotationEngine(UBYTE rs, USHORT bw, USHORT bh);
void exitRotationEngine(void);
UBYTE* getSourceBuffer(void);
UBYTE* getDestBuffer(UBYTE index);

/**
 * @brief Rotate the element in source buffer and store the results in destination buffer array.
 */
void rotateAll(void);

#endif  // ROTATION_H