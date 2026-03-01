#ifndef FIXEDPOINT_H
#define FIXEDPOINT_H

#include <exec/types.h>

// Fixed-point arithmetic for Amiga/68000 performance
// credits to: https://coronax.wordpress.com/2014/01/31/running-with-the-numbers/

#define FIXSHIFT 8

//========================================
// 16-bit fixed-point (8.8 format)
//========================================

// Convert float to fix (and back)
#define FLOATTOFIX(x) ((WORD)((x) * (1 << FIXSHIFT)))
#define FIXTOFLOAT(x) ((float)(x) / (1 << FIXSHIFT))

// Convert int to fix (and back)
#define INTTOFIX(x) ((x) << FIXSHIFT)
#define FIXTOINT(x) ((x) >> FIXSHIFT)

// Multiply and divide (unsafe - use safe_fixmult/safe_fixdiv instead)
#define FIXMULT(x, y) (((x) * (y)) >> FIXSHIFT)
#define FIXDIV(x, y) (((x) << FIXSHIFT) / (y))

// Signed WORD range limits
#define WORD_MIN -32768  // Minimum signed WORD value
#define WORD_MAX  32767  // Maximum signed WORD value

// Fixed-point range limits for signed WORD in 8.8 format
#define FIX_MIN_INFINITY -32768  // -128.0 in 8.8 fixed-point (minimum WORD value)
#define FIX_MAX_INFINITY  32512  // +127.0 in 8.8 fixed-point (127 << 8)

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

//========================================
// 32-bit fixed-point (24.8 format)
//========================================

// Uses LONG (32-bit) with 8 fractional bits, same precision as 16-bit but larger range

// Convert float to fix
#define FLOATTOFIX_LONG(x) ((LONG)((x) * (1 << FIXSHIFT)))

// Convert int to fix
#define INTTOFIX_LONG(x) ((LONG)(x) << FIXSHIFT)

// Convert between 16-bit and 32-bit fixed-point
#define FIXTOLONG(x) ((LONG)(x))  // Convert WORD 8.8 to LONG 24.8
#define LONGTOFIX(x) ((WORD)(x))  // Convert LONG 24.8 back to WORD 8.8 (with potential overflow)

// Multiply and divide
#define FIXMULT_LONG(x, y) (((x) * (y)) >> FIXSHIFT)
#define FIXDIV_LONG(x, y) (((x) << FIXSHIFT) / (y))

#endif  // FIXEDPOINT_H
