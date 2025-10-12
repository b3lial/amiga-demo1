#ifndef __GRAPHICSCONTROLLER_H__
#define __GRAPHICSCONTROLLER_H__

#include <exec/types.h>

#define COLORMAP32_BYTE_SIZE(colorAmount) ((1+colorAmount*3+1)*4)
#define COLORMAP32_LONG_SIZE(colorAmount) (1+colorAmount*3+1)
#define SPREAD(v) ((ULONG)(v) << 24 | (ULONG)(v) << 16 | (ULONG)(v) << 8 | (v))

struct Screen* createScreen(struct BitMap* b, BOOL hidden,
        WORD x, WORD y, UWORD width, UWORD height, UWORD depth,
        struct Rectangle* clip);

struct BitMap* loadBlob(const char*, UBYTE, UWORD, UWORD);
BOOL loadColorMap(char*, UWORD*, UWORD);
BOOL loadColorMap32(char* fileName, ULONG* map, UWORD colorAmount);

#endif
