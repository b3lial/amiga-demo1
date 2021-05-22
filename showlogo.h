#ifndef __SHOW_LOGO_H__
#define __SHOW_LOGO_H__

#include <exec/types.h>

#define SHOWLOGO_INIT 0
#define SHOWLOGO_STATIC 1
#define SHOWLOGO_SHUTDOWN 2

#define SHOWLOGO_BLOB_DEPTH 4
#define SHOWLOGO_BLOB_COLORS 16
#define SHOWLOGO_BLOB_WIDTH 320
#define SHOWLOGO_BLOB_HEIGHT 256

WORD fsmShowLogo(void);
void initShowLogo(void);
void exitShowLogo(void);

#endif