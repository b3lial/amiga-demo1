// Copyright 2021 Christian Ammann

#ifndef TEXTCONTROLLER_H_
#define TEXTCONTROLLER_H_

#include <exec/types.h>

#define TEXT_MOVEMENT_SPEED 3
#define MAX_CHAR_PER_LINE 20

struct CharBlob {
    UWORD xSize;
    UWORD ySize;
    WORD xPos;  // can become negative when scrolling out
    UWORD yPos;
    UWORD xPosInFont;
    UWORD yPosInFont;
    struct BitMap *oldBackground;
};

void initTextScrollEngine(char *text, UWORD firstX, UWORD firstY,
                          UWORD depth, UWORD screenWidth);
void executeTextScrollEngine(void);
void textScrollIn(void);
void textScrollOut(void);
void terminateTextScrollEngine(void);

void getCharData(char letter, struct CharBlob *charBlob);
UWORD displayCurrentCharacter(WORD xPos, WORD yPos);
void prepareForNextCharacter(void);

#endif  // TEXTCONTROLLER_H_
