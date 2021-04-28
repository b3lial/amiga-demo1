// Copyright 2021 Christian Ammann

#ifndef TEXTCONTROLLER_H_
#define TEXTCONTROLLER_H_

#include <exec/types.h>

#define TEXT_MOVEMENT_SPEED 3
#define MAX_CHAR_PER_LINE 20

struct FontInfo {
    UWORD xSize;
    UWORD ySize;
    UWORD characterPosInFontX;
    UWORD characterPosInFontY;
    struct BitMap *oldBackground;
};

void initTextScrollEngine(char *text, UWORD firstX, UWORD firstY,
                          UWORD depth, UWORD screenWidth);
void executeTextScrollEngine(void);
void terminateTextScrollEngine(void);

void getCharData(char letter, struct FontInfo *fontInfo);
UWORD displayCurrentCharacter(WORD xPos, WORD yPos);
void prepareForNextCharacter(void);

#endif  // TEXTCONTROLLER_H_
