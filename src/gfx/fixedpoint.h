#ifndef FIXEDPOINT_H
#define FIXEDPOINT_H

#include <exec/types.h>

// Fixed-point arithmetic for Amiga/68000 performance
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

#endif  // FIXEDPOINT_H
