#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <clib/intuition_protos.h>
#include <clib/graphics_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <dos/dos.h>
#include <utility/tagitem.h>

#include "graphicscontroller.h"

#include "utils/utils.h"

struct Screen* createScreen(struct BitMap* b, BOOL hidden,
                            WORD x, WORD y, UWORD width, UWORD height, UWORD depth,
                            struct Rectangle* clip) {
    UBYTE endOfLineClub = 8;
    struct TagItem screentags[11] = {
        {SA_BitMap, 0},
        {SA_Left, 0},
        {SA_Top, 0},
        {SA_Width, 0},
        {SA_Height, 0},
        {SA_Depth, 0},
        {SA_Type, CUSTOMSCREEN},
        {SA_Quiet, TRUE},
        {TAG_DONE, 0},
        {0, 0},
        {0, 0}};

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
        screentags[endOfLineClub].ti_Data = 0;
    }

    if (clip) {
        screentags[endOfLineClub].ti_Tag = SA_DClip;
        screentags[endOfLineClub].ti_Data = (ULONG)clip;
        endOfLineClub++;
        screentags[endOfLineClub].ti_Tag = TAG_DONE;
        screentags[endOfLineClub].ti_Data = 0;
    }

    return OpenScreenTagList(NULL, screentags);
}

//----------------------------------------
BOOL loadColorMap(char *fileName, UWORD *map, UWORD mapLength)
{
    LONG dataRead = 0;
    BPTR mapFileHandle = (BPTR) NULL;
    writeLogFS("Trying to load color table %s\n", fileName);

    //Open input file
    mapFileHandle = Open((CONST_STRPTR) fileName, MODE_OLDFILE);
    if (!mapFileHandle)
    {
        writeLogFS("Error: Could not read %s\n", fileName);
        return FALSE;
    }

    dataRead = Read(mapFileHandle, map, mapLength * 2);
    if (dataRead == -1)
    {
        writeLog("Error: Could not read from color map input file\n");
        Close(mapFileHandle);
        return FALSE;
    }
    writeLogFS("Read %d bytes into color table buffer\n", dataRead);

    Close(mapFileHandle);
    return TRUE;
}

//----------------------------------------
BOOL loadColorMap32(char *fileName, ULONG *map, UWORD colorAmount)
{
    ULONG *buffer = NULL;
    UWORD i;
    UBYTE r;
    UBYTE g;
    UBYTE b;

    //we need a buffer which contains the input file for further processing
    buffer = AllocMem(colorAmount * sizeof(ULONG), MEMF_ANY);
    if (!buffer)
    {
        writeLog("Error: Could not allocate memory for 32 bit color table input file buffer\n");
        goto _error_cleanup;
    }
    writeLogFS("Allocated %d bytes for 32 bit color table input file buffer\n",
               colorAmount * sizeof(ULONG));

    //reuse loadColorMap() because as a first step we need its content in a buffer
    if (!loadColorMap(fileName, (UWORD *)buffer, colorAmount * 2))
    {
        goto _error_cleanup;
    }
    writeLog("Loaded 32 bit color table\n");

    //header
    map[0] = (((ULONG)colorAmount) << 16) + 0;

    //convert rgb bytes to ulong triples
    for (i = 0; i < colorAmount; i++)
    {
        b = (UBYTE)(0x000000ff & buffer[i]);
        g = (UBYTE)((0x0000ff00 & buffer[i]) >> 8);
        r = (UBYTE)((0x00ff0000 & buffer[i]) >> 16);
        map[1 + i * 3 + 0] = SPREAD(r);
        map[1 + i * 3 + 1] = SPREAD(g);
        map[1 + i * 3 + 2] = SPREAD(b);
    }

    //null termination ulong
    map[COLORMAP32_LONG_SIZE(colorAmount) - 1] = 0;

    FreeMem(buffer, colorAmount * sizeof(ULONG));
    writeLogFS("Freeing %d bytes of 32 bit color table input file buffer\n",
               colorAmount * sizeof(ULONG));
    return TRUE;

_error_cleanup:
    if (buffer)
    {
        FreeMem(buffer, colorAmount * sizeof(ULONG));
        writeLogFS("Freeing %d bytes of 32 bit color table input file buffer\n",
                   colorAmount * sizeof(ULONG));
    }
    return FALSE;
}

//----------------------------------------
struct BitMap *loadBlob(const char *fileName, UBYTE depth, UWORD width,
                        UWORD height)
{
    LONG fileSize, dataRead = 0;
    UWORD rowSize = 0;
    UWORD destRowSize = 0;
    UWORD i = 0;
    UWORD j = 0;
    UBYTE *data;
    ULONG dataIndex = 0;

    struct BitMap *blobBitMap = NULL;
    BPTR blobFileHandle = (BPTR) NULL;
    writeLogFS("Trying to load blob %s\n", fileName);

    //Open input file
    blobFileHandle = Open((CONST_STRPTR) fileName, MODE_OLDFILE);
    if (!blobFileHandle)
    {
        writeLogFS("Error: Could not read %s\n", fileName);
        goto _error_cleanup;
    }

    //Get file size
    Seek(blobFileHandle, 0, OFFSET_END);
    fileSize = Seek(blobFileHandle, 0, OFFSET_BEGINNING);
    writeLogFS("Blob %s has file size %d\n", fileName, fileSize);

    //Copy file content to raster
    blobBitMap = AllocBitMap(width, height, depth,
                             BMF_DISPLAYABLE | BMF_CLEAR, NULL);
    if (blobBitMap == NULL)
    {
        writeLog("Error: Could not allocate bitmap\n");
        goto _error_cleanup;
    }

    /*
     * Calculate line size of input file and target
     * bitmap based. Get the delta and use it to load
     * from file to target. Thhis is neccessary because
     * row sizes differ due to memory alignment restrictions.
     */
    rowSize = width / 8;
    rowSize += (width % 8 > 0) ? 1 : 0;
    destRowSize = (blobBitMap->BytesPerRow);
    writeLogFS("Calculated input file plane size: %d\n",
               (blobBitMap->Rows) * rowSize);
    writeLogFS("Calculated target buffer plane size: %d\n",
               (blobBitMap->Rows) * (blobBitMap->BytesPerRow));

    // use the delta to load line by line from file to target bitmap
    for (i = 0; i < depth; i++)
    {
        dataIndex = 0;
        data = blobBitMap->Planes[i];
        dataRead = 0;
        for (j = 0; j < (blobBitMap->Rows); j++)
        {
            dataRead += Read(blobFileHandle, &(data[dataIndex]), rowSize);
            dataIndex += destRowSize;
            if (dataRead == -1)
            {
                writeLog("Error: Could not read from Blob input file\n");
                goto _error_cleanup;
            }
            else if (dataRead == 0)
            {
                writeLogFS("All data read at bitplane %d\n", i);
                Close(blobFileHandle);
                return blobBitMap;
            }
            else
            {
                continue;
            }

            writeLogFS("Read %d bytes for bitplane %d\n", dataRead, i);
        }
    }

    Close(blobFileHandle);
    return blobBitMap;

_error_cleanup:
    if (blobFileHandle)
    {
        Close(blobFileHandle);
    }
    if (blobBitMap)
    {
        FreeBitMap(blobBitMap);
    }
    return NULL;
}
