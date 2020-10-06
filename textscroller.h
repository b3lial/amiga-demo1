#ifndef __VIEW_BALLBLOB_H__
#define __VIEW_BALLBLOB_H__

#include <exec/types.h>
#include <proto/graphics.h>

#define TEXTSCROLLER_INIT 0
#define TEXTSCROLLER_RUNNING 1 
#define TEXTSCROLLER_SHUTDOWN 2

#define TEXTSCROLLER_VIEW_WIDTH 320
#define TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT 102
#define TEXTSCROLLER_VIEW_SPACESECTION_HEIGHT 148

#define TEXTSCROLLER_BLOB_SPACE_DEPTH  8
#define TEXTSCROLLER_BLOB_SPACE_COLORS 256
#define TEXTSCROLLER_BLOB_SPACE_WIDTH 320
#define TEXTSCROLLER_BLOB_SPACE_HEIGHT 148

#define TEXTSCROLLER_BLOB_FONT_DEPTH  3
#define TEXTSCROLLER_BLOB_FONT_COLORS 8
#define TEXTSCROLLER_BLOB_FONT_WIDTH 288
#define TEXTSCROLLER_BLOB_FONT_HEIGHT 160

WORD fsmTextScroller(void);
void initTextScroller(void);
BOOL executeTextScroller(void);
void exitTextScroller(void);

void displayText(char *text, WORD xPos, WORD yPos);
void displayCharacter(char letter, WORD *xPos, WORD *yPos);

#endif
