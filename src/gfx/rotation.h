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

#include "fixedpoint.h"

// #define NATIVE_CONVERTER

#define DEGREE_RESOLUTION 10
#define MAX_ROTATION_STEPS 36

#define MAX_BITMAP_WIDTH 320
#define MAX_BITMAP_HEIGHT 256

BOOL startRotationEngine(UBYTE rs, USHORT bw, USHORT bh);
void exitRotationEngine(void);
UBYTE* getRotationSourceBuffer(void);
UBYTE* getRotationDestinationBuffer(UBYTE index);

/**
 * @brief Rotate the element in source buffer and store the results in destination buffer array.
 */
void rotateAll(void);

#endif  // ROTATION_H