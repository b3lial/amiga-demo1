#ifndef __SHOW_LOGO_H__
#define __SHOW_LOGO_H__

#include <exec/types.h>

#define SHOWLOGO_INIT 0
#define SHOWLOGO_STATIC 1
#define SHOWLOGO_SHUTDOWN 2

WORD fsmShowLogo(void);

#endif