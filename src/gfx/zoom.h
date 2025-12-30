#ifndef ZOOM_H
#define ZOOM_H

#include <exec/types.h>

BOOL startZoomEngine(UBYTE zoomSteps, USHORT bitmapWidth, USHORT bitmapHeight);
void exitZoomEngine(void);

#endif  // ZOOM_H
