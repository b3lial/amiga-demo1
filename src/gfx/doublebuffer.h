#ifndef __DOUBLEBUFFER_H__
#define __DOUBLEBUFFER_H__

#include <exec/types.h>
#include <exec/ports.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <intuition/screens.h>

/*
 * Double buffering on a single intuition screen via AllocScreenBuffer() /
 * ChangeScreenBuffer() (intuition V39+, i.e. OS 3.0+).
 *
 * Usage per frame:
 *     dbWaitBackBuffer(&db);
 *     ... draw into DB_BACK_BITMAP(&db) or DB_BACK_RP(&db) ...
 *     dbFlip(&db);
 *
 * The screen is opened hidden (SA_Behind) with buffer 0 displayed. Draw the
 * first frame into DB_FRONT_BITMAP() and call ScreenToFront(db.screen) to
 * show it. Only flip while the screen is visible: buffer change messages
 * are tied to the display, so waiting on them for a hidden screen may block.
 */

struct DoubleBufferConfig {
    UWORD bitmapWidth;          // may be larger than the screen (borders, scroll area)
    UWORD bitmapHeight;
    UWORD depth;
    WORD screenX;               // passed through to createScreen()
    WORD screenY;
    UWORD screenWidth;
    UWORD screenHeight;
    struct Rectangle *clip;     // optional SA_DClip, NULL for none
};

struct DoubleBuffer {
    struct Screen *screen;              // single screen, one ViewPort
    struct BitMap *bitmaps[2];          // owned, allocated by openDoubleBuffer()
    struct ScreenBuffer *buffers[2];
    struct RastPort rastPorts[2];       // layerless RastPort per bitmap
    struct MsgPort *safePort;           // reply: old buffer no longer displayed
    struct MsgPort *dispPort;           // reply: new buffer now displayed
    BOOL safeToWrite;
    BOOL safeToChange;
    UBYTE backIndex;                    // buffer currently being drawn into
};

/**
 * @brief Allocates both bitmaps, opens a hidden screen and attaches two screen buffers.
 * Buffer 0 is displayed, buffer 1 is the back buffer. Cleans up after itself on failure.
 */
BOOL openDoubleBuffer(struct DoubleBuffer *db, const struct DoubleBufferConfig *cfg);

/**
 * @brief Frees screen buffers, closes the screen and frees the bitmaps.
 * Safe to call on a partially opened or already closed DoubleBuffer.
 */
void closeDoubleBuffer(struct DoubleBuffer *db);

/**
 * @brief Blocks until the back buffer is no longer displayed. Call before drawing.
 */
void dbWaitBackBuffer(struct DoubleBuffer *db);

/**
 * @brief Shows the back buffer and swaps roles. Waits until the previous flip is visible.
 */
void dbFlip(struct DoubleBuffer *db);

#define DB_BACK_BITMAP(db)  ((db)->bitmaps[(db)->backIndex])
#define DB_FRONT_BITMAP(db) ((db)->bitmaps[(db)->backIndex ^ 1])
#define DB_BACK_RP(db)      (&(db)->rastPorts[(db)->backIndex])
#define DB_FRONT_RP(db)     (&(db)->rastPorts[(db)->backIndex ^ 1])
#define DB_VIEWPORT(db)     (&(db)->screen->ViewPort)

#endif
