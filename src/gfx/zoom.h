#ifndef ZOOM_H
#define ZOOM_H

#include <exec/types.h>

#include "fixedpoint.h"

BOOL startZoomEngine(UBYTE zoomSteps, USHORT bitmapWidth, USHORT bitmapHeight);
void exitZoomEngine(void);
// Note: fixZoomFactor is in fixed-point format (use FLOATTOFIX() or INTTOFIX() to convert)
void zoomBitmap(UBYTE *source, WORD fixZoomFactor, UBYTE index);
UBYTE* getZoomDestinationBuffer(UBYTE index);

#endif  // ZOOM_H
