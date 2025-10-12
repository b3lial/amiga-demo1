#ifndef __GRAPHICSCONTROLLER_H__
#define __GRAPHICSCONTROLLER_H__

#include <exec/types.h>

#define COLORMAP32_BYTE_SIZE(colorAmount) ((1+colorAmount*3+1)*4)
#define COLORMAP32_LONG_SIZE(colorAmount) (1+colorAmount*3+1)
#define SPREAD(v) ((ULONG)(v) << 24 | (ULONG)(v) << 16 | (ULONG)(v) << 8 | (v))

struct Screen* createScreen(struct BitMap* b, BOOL hidden,
        WORD x, WORD y, UWORD width, UWORD height, UWORD depth,
        struct Rectangle* clip);

/**
 * @brief Load non-interlaced graphic blob from file system and copy it into bitplanes
 */
struct BitMap* loadBlob(const char*, UBYTE, UWORD, UWORD);

/**
 * @brief Loads a set of unsigned words from file and copies them into an array to use them as input for background color registers
 */
BOOL loadColorMap(char*, UWORD*, UWORD);

/**
 * @brief Loads a colormap from file. Input file format: Set of ULONG with (0,r,g,b), (0,r,g,b), ... Map format: First ULONG is reserved for (colorAmount,startregister), followed by (r,g,b) (each color value is a ULONG), final character in array is 0. For usage with LoadRGB32().
 */
BOOL loadColorMap32(char* fileName, ULONG* map, UWORD colorAmount);

#endif
