#ifndef __VIEW_BALLBLOB_H__
#define __VIEW_BALLBLOB_H__

#include <exec/types.h>
#include <proto/graphics.h>

#define VIEW_TEXTSCROLLER_INIT 0
#define VIEW_TEXTSCROLLER_RUNNING 1 
#define VIEW_TEXTSCROLLER_SHUTDOWN 2

#define VIEW_TEXTSCROLLER_WIDTH 320
#define VIEW_TEXTSCROLLER_HEIGHT 256

#define VIEW_TEXTSCROLLER_SPACE_DEPTH  8
#define VIEW_TEXTSCROLLER_SPACE_COLORS 256
#define VIEW_TEXTSCROLLER_SPACE_WIDTH 320
#define VIEW_TEXTSCROLLER_SPACE_HEIGHT 148

#define VIEW_TEXTSCROLLER_FONT_DEPTH  3
#define VIEW_TEXTSCROLLER_FONT_COLORS 8
#define VIEW_TEXTSCROLLER_FONT_WIDTH 288
#define VIEW_TEXTSCROLLER_FONT_HEIGHT 160

WORD fsmTextScroller(void);
void initTextScroller(void);
BOOL executeTextScroller(void);
void exitTextScroller(void);

void displayText(char *text, WORD xPos, WORD yPos);
void displayCharacter(char letter, WORD *xPos, WORD *yPos);

#endif
