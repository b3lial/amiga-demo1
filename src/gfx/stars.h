// Copyright 2021 Christian Ammann

#ifndef __STARS_H__
#define __STARS_H__

#include <exec/types.h>
#include <graphics/rastport.h>

#define STAR_MAX 100

/**
 * @brief create star array
 */
void createStars(UWORD numStars, UWORD width, UWORD height);

/**
 * @brief paint the previously generated random stars on a RastPort
 */
void paintStars(struct RastPort* rp, UWORD color, UWORD numStars, UWORD width, UWORD height);

/**
 * @brief move stars from left to right with wraparound
 */
void moveStars(UWORD numStars);

#endif
