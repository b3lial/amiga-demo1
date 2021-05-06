// Copyright 2021 Christian Ammann

#ifndef TEXTCONTROLLER_H_
#define TEXTCONTROLLER_H_

#include <exec/types.h>

#define TEXT_MOVEMENT_SPEED 8
#define TEXT_PAUSE_TIME 120
#define MAX_CHAR_PER_LINE 20

#define TEXTSCROLLER_BLOB_FONT_WIDTH 288
#define TEXTSCROLLER_BLOB_FONT_HEIGHT 160

struct CharBlob
{
    UWORD xSize;
    UWORD ySize;
    WORD xPos; // can become negative when scrolling out
    UWORD yPos;
    UWORD xPosInFont;
    UWORD yPosInFont;
    struct BitMap *oldBackground;
};

enum TEXT_CONTROL_STATE
{
    TC_SCROLL_IN,
    TC_SCROLL_PAUSE,
    TC_SCROLL_OUT,
    TC_SCROLL_FINISHED
};

// external APIs
void setStringTextController(char *text, UWORD firstX, UWORD firstY);
BOOL initTextController(struct BitMap *screen,
                        UWORD depth, UWORD screenWidth);
void executeTextController(void);
void resetTextController(void);
void exitTextController(void);
BOOL isFinishedTextController(void);

// internal functions
void textScrollIn(void);
void textScrollPause(void);
void textScrollOut(void);

void getCharData(char letter, struct CharBlob *charBlob);
UWORD displayCurrentCharacter(WORD xPos, WORD yPos);
void prepareForNextCharacter(void);

#endif // TEXTCONTROLLER_H_
