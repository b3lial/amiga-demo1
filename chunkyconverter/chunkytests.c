#include "demo1.h"

#define TEST_BITMAP_X 20
#define TEST_BITMAP_Y 20

void chunkyTests(void) {
    struct BitMap *testBitmap;
    struct p2cStruct p2c;
    UBYTE *chunkyBuffer;

    // just call asm test functions
    printf("testFunc() returned: %d\n", testFunc());
    printf("addFunc(1,2) returned: %d\n", addFunc(1, 2));

    // invoke
    testBitmap = AllocBitMap(TEST_BITMAP_X, TEST_BITMAP_Y, 8,
                             BMF_DISPLAYABLE | BMF_CLEAR, NULL);
    chunkyBuffer = AllocVec(TEST_BITMAP_X * TEST_BITMAP_Y, NULL);
    p2c.bmap = testBitmap;
    p2c.startX = 0;
    p2c.startY = 0;
    p2c.width = TEST_BITMAP_X;
    p2c.height = TEST_BITMAP_Y;
    p2c.chunkybuffer = chunkyBuffer;

    PlanarToChunkyAsm(&p2c);

    FreeVec(chunkyBuffer);
    FreeBitMap(testBitmap);
}