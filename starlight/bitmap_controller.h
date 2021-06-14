#ifndef __BITMAP_CONTROLLER_H__
#define __BITMAP_CONTROLLER_H__

#include <exec/types.h>
#include <proto/graphics.h>

struct BitMap* createBitMap(UBYTE, UWORD, UWORD);
void initBitMap(struct BitMap *newBitMap, UBYTE depth, UWORD width, UWORD height);
void cleanBitPlanes(PLANEPTR*, UBYTE, UWORD, UWORD);
void cleanBitMap(struct BitMap*);

#endif
