#ifndef __UTILS_H__
#define __UTILS_H__

#include <exec/types.h>
#include "democonstants.h"

BOOL mouseClick(void);
BOOL mouseCiaStatus(void);
BOOL writeLogFS(const char* formatString, ...);
BOOL writeArrayLog(char*, unsigned char*, UWORD);
BOOL initLog(void);
BOOL writeLog(char*);

#endif
