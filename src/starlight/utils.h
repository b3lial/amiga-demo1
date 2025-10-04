#ifndef __UTILS_H__
#define __UTILS_H__

#include "demo_constants.h"

#define STAR_MAX 100

BOOL mouseClick(void);
BOOL mouseCiaStatus(void);
BOOL writeLogFS(const char* formatString, ...);
BOOL writeArrayLog(char*, unsigned char*, UWORD);
BOOL initLog(void);
BOOL writeLog(char*);
void initStars(UWORD numStars, UWORD width, UWORD height);
void createStars(struct RastPort* rp, UWORD color, UWORD numStars, UWORD width, UWORD height);

#endif
