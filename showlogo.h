#ifndef __SHOW_LOGO_H__
#define __SHOW_LOGO_H__

#define SHOWLOGO_INIT 0
#define SHOWLOGO_STATIC 1
#define SHOWLOGO_SHUTDOWN 2

#define SHOWLOGO_BLOB_DEPTH 8
#define SHOWLOGO_BLOB_COLORS 256
#define SHOWLOGO_BLOB_WIDTH 320
#define SHOWLOGO_BLOB_HEIGHT 256

#define SHOWLOGO_BLOB_BORDER 20
#define SHOWLOGO_BLOB_WIDTH_WITH_BORDER \
    (SHOWLOGO_BLOB_WIDTH+SHOWLOGO_BLOB_BORDER)
#define SHOWLOGO_BLOB_HEIGHT_WITH_BORDER \
    (SHOWLOGO_BLOB_HEIGHT+SHOWLOGO_BLOB_BORDER)

UWORD fsmShowLogo(void);
UWORD initShowLogo(void);
void exitShowLogo(void);
void fadeInFromWhite(void);

#endif
