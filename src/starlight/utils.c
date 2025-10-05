#include <exec/types.h>
#include <graphics/rastport.h>
#include <clib/graphics_protos.h>
#include <clib/dos_protos.h>
#include <clib/alib_protos.h>
#include <dos/dos.h>
#include <hardware/cia.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"

static BOOL mousePressed = FALSE;
__far extern struct CIA ciaa;

BOOL mouseClick(void) {
    if (!mousePressed && mouseCiaStatus()) {
        mousePressed = TRUE;
        return FALSE;
    } else if (mousePressed && !mouseCiaStatus()) {
        writeLog("Mouse click detected\n");
        mousePressed = FALSE;
        return TRUE;
    } else {
        return FALSE;
    }
}

BOOL mouseCiaStatus(void) {
    if (ciaa.ciapra & CIAF_GAMEPORT0) {
        return FALSE;
    } else {
        return TRUE;
    }
}

#ifdef DEMO_DEBUG
BOOL initLog(void) {
    const char* logMessage = "Starlight Demo Logfile\n";
    BPTR logHandle = Open((CONST_STRPTR)"ram:starlight-demo.log", MODE_NEWFILE);
    if (logHandle == ((BPTR) NULL)) {
        return FALSE;
    }

    Write(logHandle, logMessage, strlen(logMessage));

    Close(logHandle);
    return TRUE;
}

/**
 * Write format string into logfile
 */
BOOL writeLogFS(const char* formatString, ...) {
    char str[DEMO_STR_MAX];
    va_list args;
    va_start(args, formatString);
    vsprintf(str, formatString, args);
    va_end(args);
    return writeLog(str);
}

/**
 * Write string into logfile
 */
BOOL writeLog(char* msg) {
    BPTR logHandle = Open((CONST_STRPTR)"ram:starlight-demo.log", MODE_OLDFILE);
    if (logHandle == ((BPTR) NULL)) {
        return FALSE;
    }

    Seek(logHandle, 0, OFFSET_END);
    Write(logHandle, msg, strlen(msg));

    Close(logHandle);
    return TRUE;
}

/**
 * Writes msg and a character array into log file. Maybe not the most
 * performant implementation but who cares.
 */
BOOL writeArrayLog(char* msg, unsigned char* array, UWORD array_length) {
    UWORD i;

    if (array_length < 2) {
        return FALSE;
    }
    if (!writeLog(msg)) {
        return FALSE;
    }

    for (i = 0; i < array_length - 1; i++) {
        if (i % 5 != 0) {
            writeLogFS("0x%x, ", array[i]);
        } else {
            writeLogFS("0x%x\n", array[i]);
        }
    }
    writeLogFS("0x%x\n", array[array_length - 1]);
    return TRUE;
}

#else
BOOL initLog(void) {
    return TRUE;
}

BOOL writeLogFS(const char* formatString, ...) {
    return TRUE;
}

BOOL writeLog(char* msg) {
    return TRUE;
}

BOOL writeArrayLog(char* msg, unsigned char* array, UWORD array_length) {
    return TRUE;
}
#endif
