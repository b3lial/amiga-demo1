#include <exec/types.h>
#include <exec/ports.h>
#include <graphics/gfx.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <string.h>

#include "doublebuffer.h"
#include "graphicscontroller.h"
#include "utils/utils.h"

//----------------------------------------
static void waitForMessage(struct MsgPort *port) {
    while (!GetMsg(port)) {
        WaitPort(port);
    }
}

//----------------------------------------
BOOL openDoubleBuffer(struct DoubleBuffer *db, const struct DoubleBufferConfig *cfg) {
    UBYTE i;

    memset(db, 0, sizeof(*db));
    db->safeToWrite = TRUE;
    db->safeToChange = TRUE;
    db->backIndex = 1;

    for (i = 0; i < 2; i++) {
        db->bitmaps[i] = AllocBitMap(cfg->bitmapWidth, cfg->bitmapHeight, cfg->depth,
                                     BMF_DISPLAYABLE | BMF_CLEAR, NULL);
        if (!db->bitmaps[i]) {
            writeLogFS("Error: Could not allocate double buffer bitmap %d\n", i);
            goto __exit_open_double_buffer;
        }
        InitRastPort(&db->rastPorts[i]);
        db->rastPorts[i].BitMap = db->bitmaps[i];
    }

    db->safePort = CreateMsgPort();
    db->dispPort = CreateMsgPort();
    if (!db->safePort || !db->dispPort) {
        writeLog("Error: Could not create double buffer message ports\n");
        goto __exit_open_double_buffer;
    }

    db->screen = createScreen(db->bitmaps[0], TRUE,
                              cfg->screenX, cfg->screenY,
                              cfg->screenWidth, cfg->screenHeight,
                              cfg->depth, cfg->clip);
    if (!db->screen) {
        writeLog("Error: Could not open double buffer screen\n");
        goto __exit_open_double_buffer;
    }

    // buffer 0 wraps the bitmap the screen was opened with, buffer 1 our second bitmap
    db->buffers[0] = AllocScreenBuffer(db->screen, NULL, SB_SCREEN_BITMAP);
    db->buffers[1] = AllocScreenBuffer(db->screen, db->bitmaps[1], 0);
    if (!db->buffers[0] || !db->buffers[1]) {
        writeLog("Error: Could not allocate screen buffers\n");
        goto __exit_open_double_buffer;
    }

    for (i = 0; i < 2; i++) {
        db->buffers[i]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort = db->safePort;
        db->buffers[i]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort = db->dispPort;
    }

    writeLog("Double buffer opened\n");
    return TRUE;

__exit_open_double_buffer:
    closeDoubleBuffer(db);
    return FALSE;
}

//----------------------------------------
void closeDoubleBuffer(struct DoubleBuffer *db) {
    UBYTE i;

    // blitter may still be writing into one of the bitmaps
    WaitBlit();

    // never free screen buffers while a change message is still outstanding
    if (!db->safeToWrite && db->safePort) {
        waitForMessage(db->safePort);
        db->safeToWrite = TRUE;
    }
    if (!db->safeToChange && db->dispPort) {
        waitForMessage(db->dispPort);
        db->safeToChange = TRUE;
    }

    if (db->screen) {
        for (i = 0; i < 2; i++) {
            if (db->buffers[i]) {
                FreeScreenBuffer(db->screen, db->buffers[i]);
                db->buffers[i] = NULL;
            }
        }
        CloseScreen(db->screen);
        db->screen = NULL;
    }

    if (db->safePort) {
        DeleteMsgPort(db->safePort);
        db->safePort = NULL;
    }
    if (db->dispPort) {
        DeleteMsgPort(db->dispPort);
        db->dispPort = NULL;
    }

    for (i = 0; i < 2; i++) {
        if (db->bitmaps[i]) {
            FreeBitMap(db->bitmaps[i]);
            db->bitmaps[i] = NULL;
        }
        db->rastPorts[i].BitMap = NULL;
    }
}

//----------------------------------------
void dbWaitBackBuffer(struct DoubleBuffer *db) {
    if (!db->safeToWrite) {
        waitForMessage(db->safePort);
        db->safeToWrite = TRUE;
    }
}

//----------------------------------------
void dbFlip(struct DoubleBuffer *db) {
    // previous flip must have reached the display before the next change
    if (!db->safeToChange) {
        waitForMessage(db->dispPort);
        db->safeToChange = TRUE;
    }

    // fails transiently, e.g. while intuition is busy with the screen
    while (!ChangeScreenBuffer(db->screen, db->buffers[db->backIndex])) {
        WaitTOF();
    }

    db->safeToChange = FALSE;
    db->safeToWrite = FALSE;
    db->backIndex ^= 1;
}
