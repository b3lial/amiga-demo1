#ifndef __CHUNKY_CONVERTER_H__
#define __CHUNKY_CONVERTER_H__

#include <exec/types.h>
#include <graphics/gfxbase.h>
#include <stdio.h>

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

void PlanarToChunkyAsm(/**__reg("a0")**/ struct p2cStruct *p2c);
void ChunkyToPlanarAsm(/**__reg("a0")**/ struct c2pStruct *c2p);
UWORD testFunc(void);
UWORD addFunc(UWORD, UWORD);

#endif