#ifndef ZOOM_H
#define ZOOM_H

#include <exec/types.h>

#include "fixedpoint.h"

BOOL startZoomEngine(UBYTE zoomSteps, USHORT bitmapWidth, USHORT bitmapHeight);
void exitZoomEngine(void);
void zoomBitmap(UBYTE *source, WORD zoomFactor, UBYTE index);
UBYTE* getZoomDestinationBuffer(UBYTE index);

#endif  // ZOOM_H
