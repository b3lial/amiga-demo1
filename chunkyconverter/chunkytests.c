#include "demo1.h"

#define TEST_BITMAP_X 20
#define TEST_BITMAP_Y 20

void chunkyTests(void) {
    struct BitMap *testBitmap;
    struct p2cStruct p2c;
    UBYTE *chunkyBuffer;
    UWORD i, j;

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
    printf("chunky buffer:\n");
    for (i = 0; i < TEST_BITMAP_X; i++) {
        printf("%02d: ", i);
        for (j = 0; j < TEST_BITMAP_Y; j++) {
            printf("%x ", chunkyBuffer[i * TEST_BITMAP_X + j]);
        }
        printf("\n");
    }

    FreeVec(chunkyBuffer);
    FreeBitMap(testBitmap);
}