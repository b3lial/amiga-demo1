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
#define FIXSHIFT 8  // shift 16 bits = scale factor 65536
#define HALFSHIFT 4
// convert float to fix (and back)
#define FLOATTOFIX(x) ((WORD)((x) * (1 << FIXSHIFT)))
#define FIXTOFLOAT(x) ((float)(x) / (1 << FIXSHIFT))
// convert int to fix (and back)
#define INTTOFIX(x) ((x) << FIXSHIFT)
#define FIXTOINT(x) ((x) >> FIXSHIFT)
// multiply and divide
#define FIXMULT(x, y) (((x) >> HALFSHIFT) * ((y) >> HALFSHIFT))
#define FIXDIV(x, y) (((x) / (y >> HALFSHIFT)) << HALFSHIFT)

#define DEGREE_RESOLUTION 10
#define DEST_BUFFER_SIZE 36

#define MAX_BITMAP_WIDTH 320
#define MAX_BITMAP_HEIGHT 256

BOOL startRotationEngine(UBYTE rs, USHORT bw, USHORT bh);
void exitRotationEngine(void);
UBYTE* getSourceBuffer(void);
UBYTE* getDestBuffer(UBYTE index);
void preCalcSinCos(UWORD lookupIndex, WORD* sinLookupX, WORD* cosLookupX,
                   WORD* sinLookupY, WORD* cosLookupY);

void rotateAll(void);
void rotate(UBYTE* dest, USHORT angle);

void convertChunkyToBitmap(UBYTE* sourceChunky, struct BitMap* destPlanar);

#endif  // ROTATION_H