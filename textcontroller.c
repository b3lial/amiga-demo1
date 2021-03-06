// Copyright 2021 Christian Ammann

#include "demo1.h"

struct BitMap *fontBlob;
struct BitMap *textDestination;
struct TextConfig **textConfigs;

// Color depth of font blob
UWORD charDepth = 0;

// Screen width
UWORD scrollControlWidth = 0;

// when complete text is on screen, pause for a short moment
UWORD pauseCounter = 0;
UWORD pauseTime = 0;

/**
 * Load font
 */
BOOL initTextController(struct BitMap *screen, UWORD depth, UWORD screenWidth)
{
    charDepth = depth;
    scrollControlWidth = screenWidth;
    textDestination = screen;
    pauseTime = TEXT_PAUSE_TIME;

    // Load font bitmap and its colors
    writeLog("Load font bitmap and colors\n");
    fontBlob = loadBlob("img/charset_final.RAW", charDepth,
                        TEXTSCROLLER_BLOB_FONT_WIDTH, TEXTSCROLLER_BLOB_FONT_HEIGHT);
    if (fontBlob == NULL)
    {
        writeLog("Error: Could not load font blob\n");
        return FALSE;
    }
    writeLogFS(
        "Font BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
        fontBlob->BytesPerRow, fontBlob->Rows, fontBlob->Flags,
        fontBlob->pad);
    return TRUE;
}

/**
 * Free font 
 */
void exitTextController(void)
{
    if (fontBlob)
    {
        FreeBitMap(fontBlob);
        fontBlob = NULL;
    }
}

/**
 * Must be called first to init the whole engine.
 * Afterwards, executeTextScroller() can be called.
 */
void setStringsTextController(struct TextConfig **configs)
{
    UBYTE i = 0;
    textConfigs = configs;
    while (textConfigs[i])
    {
        setStringTextController(textConfigs[i]);
        i++;
    }
}

/**
 * Is called for each element in text list
 */
void setStringTextController(struct TextConfig *c)
{
    // Init engines global variables
    c->currentState = TC_SCROLL_IN;
    pauseCounter = 0;
    c->charIndex = 0;
    memset(c->characters, 0, sizeof(c->characters));
    c->currentChar = 0;

    // analyse first character in text string
    getCharData(c->currentText[c->currentChar], &CURRENT_CHAR(c));
    CURRENT_CHAR(c).xPos = scrollControlWidth - CURRENT_CHAR(c).xSize;
    CURRENT_CHAR(c).yPos = c->charYPosDestination;

    // save background at character starting position
    CURRENT_CHAR(c).oldBackground = AllocBitMap(MAX_CHAR_WIDTH, MAX_CHAR_HEIGHT,
                                                charDepth, BMF_CLEAR, NULL);
    saveCharacterBackground(c);
}

/**
 * Execute text scroller engine. Should be called
 * for each new frame.
 */
void executeTextController()
{
    UBYTE i = 0;
    while (textConfigs[i])
    {
        switch (textConfigs[i]->currentState)
        {
        case TC_SCROLL_IN:
            textScrollIn(textConfigs[i]);
            break;
        case TC_SCROLL_PAUSE:
            textScrollPause(textConfigs[i]);
            break;
        case TC_SCROLL_OUT:
            textScrollOut(textConfigs[i]);
            break;
        case TC_SCROLL_FINISHED:
            break;
        }
        i++;
    }
}

void textScrollIn(struct TextConfig *c)
{
    // check whether every char was moved at its position
    if (c->currentText[c->currentChar] == 0)
    {
        c->maxCharIndex = c->charIndex;
        c->charIndex = 0;
        c->charXPosDestination = 0;
        c->currentState = TC_SCROLL_PAUSE;
        return;
    }

    // restore previously saved background and character position
    restorePreviousBackground(c);

    // move character to next position
    CURRENT_CHAR(c).xPos -= TEXT_MOVEMENT_SPEED;
    if (CURRENT_CHAR(c).xPos < c->charXPosDestination)
    {
        CURRENT_CHAR(c).xPos = c->charXPosDestination;
    }

    // save background there
    saveCharacterBackground(c);

    // blit character on screen
    displayCurrentCharacter(c);

    // finally, check whether we have to switch to next letter
    if (CURRENT_CHAR(c).xPos <= c->charXPosDestination)
    {
        prepareForNextCharacter(c);
    }
}

void pauseTimeTextController(UWORD newPauseTime)
{
    pauseTime = newPauseTime;
}

void textScrollPause(struct TextConfig *textConfig)
{
    if (pauseCounter >= pauseTime)
    {
        textConfig->currentState = TC_SCROLL_OUT;
        return;
    }

    pauseCounter++;
}

void textScrollOut(struct TextConfig *c)
{
    // check whether every char was scrolled out and we are finished
    if (c->charIndex > c->maxCharIndex)
    {
        c->currentState = TC_SCROLL_FINISHED;
        return;
    }

    // restore previously saved background and character position
    restorePreviousBackground(c);

    // char reached left side, delete it and switch to next char
    if (CURRENT_CHAR(c).xPos == c->charXPosDestination)
    {
        c->charIndex++;
        return;
    }

    // move character to next position
    CURRENT_CHAR(c).xPos -= TEXT_MOVEMENT_SPEED;
    if (CURRENT_CHAR(c).xPos < 0)
    {
        CURRENT_CHAR(c).xPos = 0;
    }

    // save background there
    saveCharacterBackground(c);

    // blit character on screen
    displayCurrentCharacter(c);
}

/**
 * Returns true if every string TextConfig 
 * is in state TC_SCROLL_FINISHED
 */
BOOL isFinishedTextController(void)
{
    BOOL allFinished = TRUE;
    UBYTE i = 0;
    while (textConfigs[i])
    {
        if (textConfigs[i]->currentState != TC_SCROLL_FINISHED)
        {
            allFinished = FALSE;
        }
        i++;
    }
    return allFinished;
}

/*
 * Search in text string for next non-whitespace
 * character and initialize its destination position
 */
void prepareForNextCharacter(struct TextConfig *c)
{
    char letter = 0;
    c->charXPosDestination += (CURRENT_CHAR(c).xSize + 5);

    // skip space
    c->currentChar++;
    letter = tolower(c->currentText[c->currentChar]);
    // reach end of string
    if (letter == 0)
    {
        return;
    }

    while (letter < 'a' || letter > 'z')
    {
        c->charXPosDestination += 15;
        c->currentChar++;
        letter = tolower(c->currentText[c->currentChar]);

        // reach end of string
        if (letter == 0)
        {
            return;
        }
    }

    // found next character, prepare everything for his arrival
    c->charIndex++;
    getCharData(letter, &CURRENT_CHAR(c));
    CURRENT_CHAR(c).oldBackground = AllocBitMap(50, 50,
                                                charDepth, BMF_CLEAR, NULL);
    CURRENT_CHAR(c).xPos = scrollControlWidth - CURRENT_CHAR(c).xSize;
    CURRENT_CHAR(c).yPos = c->charYPosDestination;

    // save background at character starting position
    saveCharacterBackground(c);
}

/**
 * Free the character background backup bitmaps of all text configs
 */
void resetTextController(void)
{
    UBYTE i = 0;
    while (textConfigs[i])
    {
        resetTextConfig(textConfigs[i]);
        i++;
    }
}

/**
 * Free the character background backup bitmaps of text config
 */
void resetTextConfig(struct TextConfig *textConfig)
{
    UBYTE i = 0;
    for (; i < MAX_CHAR_PER_LINE; i++)
    {
        if (textConfig->characters[i].oldBackground)
        {
            FreeBitMap(textConfig->characters[i].oldBackground);
            textConfig->characters[i].oldBackground = NULL;
        }
    }
}

/**
 * Print a character on screen. Return position of next
 * character
 */
UWORD displayCurrentCharacter(struct TextConfig *c)
{
    /*
	 * Don't erase background if character rectangle (B) is blitted into destination (C,D)
	 * Therefore, we use minterm: BC+NBC+BNC -> 1110xxxx -> 0xE0
	 */
    BltBitMap(fontBlob,
              CURRENT_CHAR(c).xPosInFont, CURRENT_CHAR(c).yPosInFont,
              textDestination,
              CURRENT_CHAR(c).xPos, CURRENT_CHAR(c).yPos,
              CURRENT_CHAR(c).xSize, CURRENT_CHAR(c).ySize,
              0xC0, 0xff, 0);
    return (UWORD)(c->characters[c->charIndex].xPos + c->characters[c->charIndex].xSize + 5);
}

/**
 * Restore a piece which was previously stored in a TextConfig
 * object
 */
void restorePreviousBackground(struct TextConfig *c)
{
    BltBitMap(CURRENT_CHAR(c).oldBackground,
              0, 0, textDestination,
              CURRENT_CHAR(c).xPos, CURRENT_CHAR(c).yPos,
              CURRENT_CHAR(c).xSize, CURRENT_CHAR(c).ySize,
              0xC0, 0xff, 0);
}

/**
 * Backup a part of the background before blitting a character
 * there to be able to restore it later 
 */
void saveCharacterBackground(struct TextConfig *c)
{
    BltBitMap(textDestination, CURRENT_CHAR(c).xPos,
              CURRENT_CHAR(c).yPos,
              CURRENT_CHAR(c).oldBackground, 0, 0,
              CURRENT_CHAR(c).xSize,
              CURRENT_CHAR(c).ySize, 0xC0,
              0xff, 0);
}
