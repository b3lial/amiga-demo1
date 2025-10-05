// Copyright 2021 Christian Ammann

#ifndef TEXTSCROLLER_H__
#define TEXTSCROLLER_H__

#include <exec/types.h>
#include "demo_constants.h"

#define TEXT_LIST_SIZE 4

#define TEXTSCROLLER_INIT 0
#define TEXTSCROLLER_MSG_1 1
#define TEXTSCROLLER_MSG_2 2
#define TEXTSCROLLER_MSG_3 3
#define TEXTSCROLLER_MSG_4 4
#define TEXTSCROLLER_FADE_WHITE 5

// reserve additional space as scroll in area
#define TEXTSCROLLER_VIEW_TEXTSECTION_WIDTH \
    (TEXTSCROLLER_VIEW_WIDTH + MAX_CHAR_WIDTH * 2)

#define TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT 125
#define TEXTSCROLLER_VIEW_SPACESECTION_HEIGHT 125

#define TEXTSCROLLER_BLOB_SPACE_DEPTH 8
#define TEXTSCROLLER_BLOB_SPACE_COLORS 256
#define TEXTSCROLLER_BLOB_SPACE_WIDTH 320
#define TEXTSCROLLER_BLOB_SPACE_HEIGHT 125

#define TEXTSCROLLER_BLOB_FONT_DEPTH 3
#define TEXTSCROLLER_BLOB_FONT_COLORS 8

UWORD fsmTextScroller(void);
UWORD initTextScroller(void);
void exitTextScroller(void);
void fadeToWhite(void);
BOOL hasFadeToWhiteFinished(void);

#endif  // TEXTSCROLLER_H__
