// Copyright 2021 Christian Ammann

#ifndef TEXTCONTROLLER_H_
#define TEXTCONTROLLER_H_

#include <exec/types.h>
#include "demo_constants.h"

#define TEXTSCROLLER_BLOB_FONT_WIDTH 288
#define TEXTSCROLLER_BLOB_FONT_HEIGHT 160

#define TEXT_MOVEMENT_SPEED 8
#define TEXT_PAUSE_TIME 120
#define TEXT_LIST_SIZE 4
#define MAX_CHAR_PER_LINE (TEXTSCROLLER_VIEW_WIDTH / MAX_CHAR_WIDTH + 1)


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

// Public configuration struct - caller specifies what to display
struct TextConfig
{
    char *currentText;
    UWORD charXPosDestination;
    UWORD charYPosDestination;
};

// external APIs
BOOL initTextController(struct BitMap *screen,
                        UWORD depth, UWORD screenWidth);
void setStringsTextController(struct TextConfig** configs);
void executeTextController(void);
void resetTextController(void);
void exitTextController(void);
BOOL isFinishedTextController(void);
void pauseTimeTextController(UWORD);


#endif // TEXTCONTROLLER_H_
