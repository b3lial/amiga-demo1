#include "demo1.h"

#define TEST_BITMAP_X 20
#define TEST_BITMAP_Y 20

void chunkyTests(void)
{
    struct BitMap *sourceBitmap;
    struct BitMap *destinationBitmap;

    struct p2cStruct p2c = {0};
    struct c2pStruct c2p = {0};

    struct RastPort rastPort = {0};
    UBYTE *chunkyBuffer;
    UWORD i, j;

    // alloc memory for chunky and planar buffers
    sourceBitmap = AllocBitMap(TEST_BITMAP_X, TEST_BITMAP_Y, 8,
                               BMF_DISPLAYABLE | BMF_CLEAR, NULL);
    destinationBitmap = AllocBitMap(TEST_BITMAP_X, TEST_BITMAP_Y, 8,
                                    BMF_DISPLAYABLE | BMF_CLEAR, NULL);
    chunkyBuffer = AllocVec(TEST_BITMAP_X * TEST_BITMAP_Y, NULL);

    // paint in the planar buffer
    InitRastPort(&rastPort);
    rastPort.BitMap = sourceBitmap;
    SetAPen(&rastPort, 6);
    RectFill(&rastPort, 7, 7, 13, 13);

    // convert planar buffer to chunky
    p2c.bmap = sourceBitmap;
    p2c.startX = 0;
    p2c.startY = 0;
    p2c.width = TEST_BITMAP_X;
    p2c.height = TEST_BITMAP_Y;
    p2c.chunkybuffer = chunkyBuffer;
    PlanarToChunkyAsm(&p2c);

    // convert chunky back to planar
    c2p.bmap = destinationBitmap;
    c2p.startX = 0;
    c2p.startY = 0;
    c2p.width = TEST_BITMAP_X;
    c2p.height = TEST_BITMAP_Y;
    c2p.chunkybuffer = chunkyBuffer;
    ChunkyToPlanarAsm(&c2p);

    // clean chunky before copying back
    memset(chunkyBuffer, 0, TEST_BITMAP_X * TEST_BITMAP_Y);
    // convert planar buffer to chunky
    p2c.bmap = destinationBitmap;
    p2c.startX = 0;
    p2c.startY = 0;
    p2c.width = TEST_BITMAP_X;
    p2c.height = TEST_BITMAP_Y;
    p2c.chunkybuffer = chunkyBuffer;
    PlanarToChunkyAsm(&p2c);

    // print content of chunky buffer on screen
    printf("chunky buffer:\n");
    for (i = 0; i < TEST_BITMAP_X; i++)
    {
        printf("%02d: ", i);
        for (j = 0; j < TEST_BITMAP_Y; j++)
        {
            printf("%x ", chunkyBuffer[i * TEST_BITMAP_X + j]);
        }
        printf("\n");
    }

    // free memory and quit
    FreeVec(chunkyBuffer);
    FreeBitMap(sourceBitmap);
    FreeBitMap(destinationBitmap);
}