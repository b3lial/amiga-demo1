#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <exec/types.h>
#include <graphics/copper.h>
#include <graphics/displayinfo.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/gfxnodes.h>
#include <graphics/videocontrol.h>
#include <graphics/view.h>
#include <libraries/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/tagitem.h>

#include "starlight/starlight.h"

/**
 * Allocates memory for bitmap and its bitplanes
 */
struct BitMap* createBitMap(UBYTE depth, UWORD width, UWORD height){
    struct BitMap* newBitMap;
    BYTE i,j = 0;

    writeLogFS("Allocating memory for %dx%dx%d BitMap\n", depth, width, height);
    //Alloc BitMap structure and init with zero 
    newBitMap = AllocMem(sizeof(struct BitMap), MEMF_CHIP);
    if(!newBitMap){
        writeLog("Error: Could not allocate Bitmap memory\n");
        return NULL;
    }
    memset(newBitMap, 0, sizeof(struct BitMap));
    InitBitMap(newBitMap, depth, width, height);

    for(i=0; i<depth; i++){
        newBitMap->Planes[i] = (PLANEPTR) AllocRaster(
                (newBitMap->BytesPerRow) * 8, newBitMap->Rows);
        if(newBitMap->Planes[i] == NULL){
            //error, free previously allocated memory
            writeLogFS("Error: Could not allocate Bitplane %d memory\n", i);
            for(j=i-1; j>=0; j--){
                FreeRaster(newBitMap->Planes[j], (newBitMap->BytesPerRow) * 8, 
                    newBitMap->Rows);
            }
            FreeMem(newBitMap, sizeof(struct BitMap));
            return NULL;
        }
    }

    return newBitMap;
}

void initBitMap(struct BitMap *newBitMap, UBYTE depth, UWORD width, UWORD height){
    BYTE i,j = 0;
    memset(newBitMap, 0, sizeof(struct BitMap));
    InitBitMap(newBitMap, depth, width, height);

    for(i=0; i<depth; i++){
        newBitMap->Planes[i] = (PLANEPTR) AllocRaster(
                (newBitMap->BytesPerRow) * 8, newBitMap->Rows);
        if(newBitMap->Planes[i] == NULL){
            //error, free previously allocated memory
            writeLogFS("Error: Could not allocate Bitplane %d memory\n", i);
            for(j=i-1; j>=0; j--){
                FreeRaster(newBitMap->Planes[j], (newBitMap->BytesPerRow) * 8, 
                    newBitMap->Rows);
            }
            return;
        }
    }
}

/**
 * Free BitMMap memory and its BitPlanes
 */
void cleanBitMap(struct BitMap* bitmap){
    cleanBitPlanes(bitmap->Planes, bitmap->Depth, (bitmap->BytesPerRow)*8,
            bitmap->Rows);
    writeLogFS("Freeing %d bytes of BitMap memory\n", sizeof(struct BitMap));
    FreeMem(bitmap, sizeof(struct BitMap));
}

/**
 * Use FreeRaster to free an array of BitPlane memory
 */
void cleanBitPlanes(PLANEPTR* bmPlanes, UBYTE bmDepth, 
        UWORD bmWidth, UWORD bmHeight)
{
    UBYTE i=0;
    for(i=0; i<bmDepth; i++){
        if((bmPlanes[i]) != NULL){
            writeLogFS("Freeing BitPlane %d with size of %d bytes\n", 
                    i, bmWidth*bmHeight/8);
            FreeRaster(bmPlanes[i], bmWidth, bmHeight);
            bmPlanes[i] = NULL;
        }
    }
}
