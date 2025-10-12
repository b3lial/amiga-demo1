// Copyright 2021 Christian Ammann

#ifndef TEXTCONTROLLER_H_
#define TEXTCONTROLLER_H_

#include <exec/types.h>
#include "democonstants.h"

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
/**
 * @brief Load font and allocate scroll states
 */
BOOL startTextController(struct BitMap *screen,
                        UWORD depth, UWORD screenWidth);

/**
 * @brief Configure text strings for scrolling. Must be called after startTextController() and before executeTextController(). Automatically resets previous state before configuring new texts. pauseTime: pause duration in frames (0 = use default TEXT_PAUSE_TIME)
 */
void configureTextController(struct TextConfig** configs, UWORD pauseTime);

/**
 * @brief Execute text scroller engine. Should be called for each new frame.
 */
void executeTextController(void);

/**
 * @brief Free font, scroll states, and character background bitmaps
 */
void exitTextController(void);

/**
 * @brief Returns true if every active text config is in state TC_SCROLL_FINISHED
 */
BOOL isFinishedTextController(void);


#endif // TEXTCONTROLLER_H_
