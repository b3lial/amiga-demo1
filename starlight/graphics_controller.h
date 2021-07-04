#ifndef __GRAPHICS_CONTROLLER_H__
#define __GRAPHICS_CONTROLLER_H__

struct Screen* createScreen(struct BitMap* b, BOOL hidden, 
        WORD x, WORD y, UWORD width, UWORD height, UWORD depth, 
        struct Rectangle* clip);

#endif
