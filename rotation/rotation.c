#include "demo1.h"

// buffers for chunky data
UBYTE *srcBuffer;
UBYTE *destBuffer[DEST_BUFFER_SIZE] = {0};

UBYTE rotationSteps = 0;
USHORT bitmapWidth = 0;
USHORT bitmapHeight = 0;

// cosinus lookup table in fix point format
// generated by lookup_cos.py
int cosLookup[] = {
    FLOATTOFIX(1.0),
    FLOATTOFIX(0.9848),
    FLOATTOFIX(0.9397),
    FLOATTOFIX(0.866),
    FLOATTOFIX(0.766),
    FLOATTOFIX(0.6428),
    FLOATTOFIX(0.5),
    FLOATTOFIX(0.342),
    FLOATTOFIX(0.1736),
    FLOATTOFIX(0.0),
    FLOATTOFIX(-0.1736),
    FLOATTOFIX(-0.342),
    FLOATTOFIX(-0.5),
    FLOATTOFIX(-0.6428),
    FLOATTOFIX(-0.766),
    FLOATTOFIX(-0.866),
    FLOATTOFIX(-0.9397),
    FLOATTOFIX(-0.9848),
    FLOATTOFIX(-1.0),
    FLOATTOFIX(-0.9848),
    FLOATTOFIX(-0.9397),
    FLOATTOFIX(-0.866),
    FLOATTOFIX(-0.766),
    FLOATTOFIX(-0.6428),
    FLOATTOFIX(-0.5),
    FLOATTOFIX(-0.342),
    FLOATTOFIX(-0.1736),
    FLOATTOFIX(-0.0),
    FLOATTOFIX(0.1736),
    FLOATTOFIX(0.342),
    FLOATTOFIX(0.5),
    FLOATTOFIX(0.6428),
    FLOATTOFIX(0.766),
    FLOATTOFIX(0.866),
    FLOATTOFIX(0.9397),
    FLOATTOFIX(0.9848)};

// sinus lookup table in fix point format
// generated by lookup_sin.py
int sinLookup[] = {
    FLOATTOFIX(0.0),
    FLOATTOFIX(0.1736),
    FLOATTOFIX(0.342),
    FLOATTOFIX(0.5),
    FLOATTOFIX(0.6428),
    FLOATTOFIX(0.766),
    FLOATTOFIX(0.866),
    FLOATTOFIX(0.9397),
    FLOATTOFIX(0.9848),
    FLOATTOFIX(1.0),
    FLOATTOFIX(0.9848),
    FLOATTOFIX(0.9397),
    FLOATTOFIX(0.866),
    FLOATTOFIX(0.766),
    FLOATTOFIX(0.6428),
    FLOATTOFIX(0.5),
    FLOATTOFIX(0.342),
    FLOATTOFIX(0.1736),
    FLOATTOFIX(0.0),
    FLOATTOFIX(-0.1736),
    FLOATTOFIX(-0.342),
    FLOATTOFIX(-0.5),
    FLOATTOFIX(-0.6428),
    FLOATTOFIX(-0.766),
    FLOATTOFIX(-0.866),
    FLOATTOFIX(-0.9397),
    FLOATTOFIX(-0.9848),
    FLOATTOFIX(-1.0),
    FLOATTOFIX(-0.9848),
    FLOATTOFIX(-0.9397),
    FLOATTOFIX(-0.866),
    FLOATTOFIX(-0.766),
    FLOATTOFIX(-0.6428),
    FLOATTOFIX(-0.5),
    FLOATTOFIX(-0.342),
    FLOATTOFIX(-0.1736)};

BOOL initRotationEngine(UBYTE rs, USHORT bw, USHORT bh) {
    rotationSteps = rs;
    bitmapWidth = bw;
    bitmapHeight = bh;
    return allocateChunkyBuffer();
}

// apply rotation matrix
// multiplication with y is precalculated
void rotatePixel(int dest_x, int *src_x, int *src_y,
                 int y_mult_sin, int y_mult_cos,
                 UWORD i) {
    int f_x = INTTOFIX(dest_x);
    *src_x = FIXTOINT(FIXMULT(f_x, cosLookup[i]) - y_mult_sin);
    *src_y = -FIXTOINT(FIXMULT(f_x, sinLookup[i]) + y_mult_cos);
}

/**
 * Rotate the element in source buffer and store the results
 * in destination buffer array.
 */
void rotateAll() {
    USHORT angle = 0;
    UBYTE i = 0;

    for (i = 0; i < rotationSteps; i++) {
        printf("Rotating angle %d of %d\n", angle, i);
        rotate(destBuffer[i], angle);
        angle += (360 / rotationSteps);  // 360 degrees / number of steps == rotation degree
    }
    return;
}

/**
 * Gets a source chunky buffer, rotates it by degree value
 * in RotationData and writes the result into destination
 * chunky buffer. Rotation is performed via matrix multiplcation
 * in fix point format.
 */
void rotate(UBYTE *dest, USHORT angle) {
    int x, y = 0;
    int src_index, dest_index = 0;
    int dest_x, dest_y = 0;
    int src_x, src_y = 0;
    int y_mult_sin, y_mult_cos = 0;
    UWORD lookupIndex;

    // in this case, we can simply perform a copy
    if (angle == 360 || angle == 0) {
        CopyMem(srcBuffer, dest, bitmapWidth * bitmapHeight);
        return;
    }

    // iterate over destination array
    lookupIndex = (360 - angle) / DEGREE_RESOLUTION;
    for (y = 0; y < bitmapHeight; y++) {
        // precalculate these values to speed things up
        dest_y = (bitmapHeight / 2) - y;
        y_mult_sin = FIXMULT(INTTOFIX(dest_y), sinLookup[lookupIndex]);
        y_mult_cos = FIXMULT(INTTOFIX(dest_y), cosLookup[lookupIndex]);

        for (x = 0; x < bitmapWidth; x++) {
            // calculate src x/y coordinates
            dest_x = x - (bitmapWidth / 2);
            rotatePixel(dest_x, &src_x, &src_y,
                        y_mult_sin, y_mult_cos, lookupIndex);

            // convert coordinates back to array indexes
            // so we can move the rotated pixel to its new position
            dest_index = x + y * bitmapWidth;
            src_index = (src_x + (bitmapWidth / 2)) +
                        ((src_y + (bitmapHeight / 2)) * bitmapWidth);

            if (angle == 50 && y == 0 && x == 223) {
                printf("x: %d, y: %d\n", x, y);
                printf("dest_x: %d, dest_y: %d\n", dest_x, dest_y);
                printf("src_x: %d, src_y: %d\n", src_x, src_y);
                printf("lookupIndex: %d\n", lookupIndex);
                printf("src_index: %d, dest_index: %d\n", src_index, dest_index);
                printf("srcBuffer[src_index]: %d\n", srcBuffer[src_index]);
                printf("destBuffer[i][dest_index]: %d\n", dest[dest_index]);
                printf("bitmapWidth: %d, bitmapHeight: %d\n", bitmapWidth, bitmapHeight);
            }

            // verify x outofbounds
            if (src_x < -(bitmapWidth / 2) || src_x >= bitmapWidth / 2) {
                continue;
            }
            // verify y outofbounds
            if (src_y < -(bitmapHeight / 2) || src_y >= bitmapHeight / 2) {
                continue;
            }
            dest[dest_index] = srcBuffer[src_index];
        }
    }
}

/**
 * Allocate:
 * - source chunky buffer: contains the object we want to rotate
 * - destination chunky buffer array: contain the rotated objects
 */
BOOL allocateChunkyBuffer(void) {
    BYTE i = 0;

    if (rotationSteps == 0 || rotationSteps > DEST_BUFFER_SIZE) {
        printf("Error: Invalid destination buffer size %d\n", rotationSteps);
        goto _exit_chunky_source_allocation_error;
    }

    // allocate memory for chunky buffer
    srcBuffer = AllocVec(bitmapWidth * bitmapHeight, MEMF_FAST | MEMF_CLEAR);
    if (!srcBuffer) {
        printf("Error: Could not allocate memory for source chunky buffer\n");
        goto _exit_chunky_source_allocation_error;
    }

    for (i = 0; i < rotationSteps; i++) {
        destBuffer[i] = AllocVec(bitmapWidth * bitmapHeight, MEMF_FAST | MEMF_CLEAR);
        if (!(destBuffer[i])) {
            printf("Error: Could not allocate memory for destination chunky buffer array\n");
            goto _exit_chunky_source_allocation_rollback;
        }
    }

    return TRUE;

_exit_chunky_source_allocation_rollback:
    for (i -= 1; i >= 0; i--) {
        FreeVec(destBuffer[i]);
    }
_exit_chunky_source_allocation_error:
    return FALSE;
}

/**
 * Free allocated chunky buffers
 */
void freeChunkyBuffer(void) {
    BYTE i = 0;

    FreeVec(srcBuffer);
    for (i = 0; i < rotationSteps; i++) {
        FreeVec(destBuffer[i]);
    }
}

UBYTE *getSourceBuffer(void) {
    return srcBuffer;
}

UBYTE *getDestBuffer(UBYTE index) {
    return destBuffer[index];
}