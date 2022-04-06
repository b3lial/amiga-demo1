#ifndef __CHUNKY_CONVERTER_H__
#define __CHUNKY_CONVERTER_H__

#include <exec/types.h>
#include <graphics/gfxbase.h>

struct p2cStruct {
    struct BitMap *bmap;
    UWORD startX, startY, width, height;
    UBYTE *chunkybuffer;
};

struct c2pStruct {
    struct BitMap *bmap;
    UWORD startX, startY, width, height;
    UBYTE *chunkybuffer;
};

void PlanarToChunkyAsm(register __a0 struct p2cStruct *);
void ChunkyToPlanarAsm(register __a0 struct c2pStruct *);

#endif