// Copyright 2021 Christian Ammann

#ifndef TEXTCONTROLLER_H_
#define TEXTCONTROLLER_H_

#include <exec/types.h>

#define TEXT_MOVEMENT_SPEED 8
#define TEXT_PAUSE_TIME 120
#define MAX_CHAR_PER_LINE 20

#define TEXTSCROLLER_BLOB_FONT_WIDTH 288
#define TEXTSCROLLER_BLOB_FONT_HEIGHT 160

enum TEXT_CONTROL_STATE
{
    TC_SCROLL_IN,
    TC_SCROLL_PAUSE,
    TC_SCROLL_OUT,
    TC_SCROLL_FINISHED
};

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

struct TextConfig
{
    /*
    * Contains the text which is displayed and 
    * an index to current char
    */
    char *currentText;
    UWORD currentChar;

    // At which position do we want to move the current character
    UWORD charXPosDestination;
    UWORD charYPosDestination;

    // contains data of characters blitted on screen
    UBYTE charIndex;
    UBYTE maxCharIndex;
    struct CharBlob characters[MAX_CHAR_PER_LINE];

    /*
    * When the effect starts, a sequence of characters is moved from
    * right to left on screen. When each char is at its position, they
    * scoll out from right to left. In between, animation is paused
    *  for short amount of time
    */
    enum TEXT_CONTROL_STATE currentState;
};

// external APIs
void setStringTextController(struct TextConfig* config);
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
