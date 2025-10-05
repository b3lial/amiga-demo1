// Copyright 2021 Christian Ammann

#ifndef __STARS_H__
#define __STARS_H__

#include <exec/types.h>
#include <graphics/rastport.h>

#define STAR_MAX 100

void initStars(UWORD numStars, UWORD width, UWORD height);
void createStars(struct RastPort* rp, UWORD color, UWORD numStars, UWORD width, UWORD height);

#endif
