#include "demo1.h"

struct Screen* createScreen(struct BitMap* b, BOOL hidden,
                            WORD x, WORD y, UWORD width, UWORD height, UWORD depth,
                            struct Rectangle* clip) {
    UBYTE endOfLineClub = 8;
    struct TagItem screentags[11] = {
        {SA_BitMap, NULL},
        {SA_Left, 0},
        {SA_Top, 0},
        {SA_Width, 0},
        {SA_Height, 0},
        {SA_Depth, 0},
        {SA_Type, CUSTOMSCREEN},
        {SA_Quiet, TRUE},
        {TAG_DONE, NULL},
        {NULL, NULL},
        {NULL, NULL}};

    screentags[0].ti_Data = (ULONG)b;
    screentags[1].ti_Data = x;
    screentags[2].ti_Data = y;
    screentags[3].ti_Data = width;
    screentags[4].ti_Data = height;
    screentags[5].ti_Data = depth;

    if (hidden) {
        screentags[endOfLineClub].ti_Tag = SA_Behind;
        screentags[endOfLineClub].ti_Data = TRUE;
        endOfLineClub++;
        screentags[endOfLineClub].ti_Tag = TAG_DONE;
        screentags[endOfLineClub].ti_Data = NULL;
    }

    if (clip) {
        screentags[endOfLineClub].ti_Tag = SA_DClip;
        screentags[endOfLineClub].ti_Data = (ULONG)clip;
        endOfLineClub++;
        screentags[endOfLineClub].ti_Tag = TAG_DONE;
        screentags[endOfLineClub].ti_Data = NULL;
    }

    return OpenScreenTagList(NULL, screentags);
}
