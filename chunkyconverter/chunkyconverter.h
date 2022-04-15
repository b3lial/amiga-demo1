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

void PlanarToChunkyAsm(struct p2cStruct *p2c __asm("a0"));
void ChunkyToPlanarAsm(struct c2pStruct *c2p __asm("a0"));
void chunkyTests(void);

#endif