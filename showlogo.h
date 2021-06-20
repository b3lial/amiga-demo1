#ifndef __SHOW_LOGO_H__
#define __SHOW_LOGO_H__

#include <exec/types.h>

#define SHOWLOGO_INIT 0
#define SHOWLOGO_STATIC 1
#define SHOWLOGO_SHUTDOWN 2

#define SHOWLOGO_BLOB_DEPTH 8
#define SHOWLOGO_BLOB_COLORS 256
#define SHOWLOGO_BLOB_WIDTH 320
#define SHOWLOGO_BLOB_HEIGHT 256
#define SHOWLOGO_LOGO_HEIGHT 200

#define SHOWLOGO_BORDER_DEPTH 1
#define SHOWLOGO_BORDER_COLORS 2
#define SHOWLOGO_BORDER_WIDTH 320
#define SHOWLOGO_BORDER_HEIGHT 22

WORD fsmShowLogo(void);
void initShowLogo(void);
void exitShowLogo(void);
void fadeInFromWhite(void);
BOOL hasFadeInFromWhiteFinished(void);

#endif
