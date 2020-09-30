#ifndef __VIEW_BALLBLOB_H__
#define __VIEW_BALLBLOB_H__

#include <exec/types.h>
#include <proto/graphics.h>

#define VIEW_TEXTSCROLLER_INIT 0
#define VIEW_TEXTSCROLLER_RUNNING 1 
#define VIEW_TEXTSCROLLER_SHUTDOWN 2

#define VIEW_TEXTSCROLLER_DEPTH  3 
#define VIEW_TEXTSCROLLER_COLORS 8 
#define VIEW_TEXTSCROLLER_WIDTH  320
#define VIEW_TEXTSCROLLER_HEIGHT 256

#define VIEW_TEXTSCROLLER_BALL_WIDTH 207 //24  
#define VIEW_TEXTSCROLLER_BALL_HEIGHT 207 //207 

WORD fsmTextScroller(void);
void initTextScroller(void);
BOOL executeTextScroller(void);
void exitTextScroller(void);

#endif
