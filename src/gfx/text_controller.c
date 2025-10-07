// Copyright 2021 Christian Ammann

#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <clib/graphics_protos.h>
#include <clib/exec_protos.h>
#include <ctype.h>
#include <string.h>

#include "text_controller.h"

#include "utils/utils.h"
#include "gfx/blob_controller.h"

struct TextControllerContext {
    struct BitMap *fontBlob;
    struct BitMap *textDestination;
    struct TextConfig **textConfigs;
    UWORD charDepth;
    UWORD scrollControlWidth;
    UWORD pauseCounter;
    UWORD pauseTime;
};

static struct TextControllerContext ctx = {
    .fontBlob = NULL,
    .textDestination = NULL,
    .textConfigs = NULL,
    .charDepth = 0,
    .scrollControlWidth = 0,
    .pauseCounter = 0,
    .pauseTime = 0
};

/**
 * Load font
 */
BOOL initTextController(struct BitMap *screen, UWORD depth, UWORD screenWidth)
{
    ctx.charDepth = depth;
    ctx.scrollControlWidth = screenWidth;
    ctx.textDestination = screen;
    ctx.pauseTime = TEXT_PAUSE_TIME;

    // Load font bitmap and its colors
    writeLog("Load font bitmap and colors\n");
    ctx.fontBlob = loadBlob("img/charset_final.RAW", ctx.charDepth,
                        TEXTSCROLLER_BLOB_FONT_WIDTH, TEXTSCROLLER_BLOB_FONT_HEIGHT);
    if (ctx.fontBlob == NULL)
    {
        writeLog("Error: Could not load font blob\n");
        return FALSE;
    }
    writeLogFS(
        "Font BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
        ctx.fontBlob->BytesPerRow, ctx.fontBlob->Rows, ctx.fontBlob->Flags,
        ctx.fontBlob->pad);
    return TRUE;
}

/**
 * Free font 
 */
void exitTextController(void)
{
    if (ctx.fontBlob)
    {
        FreeBitMap(ctx.fontBlob);
        ctx.fontBlob = NULL;
    }
}

/**
 * Must be called first to init the whole engine.
 * Afterwards, executeTextScroller() can be called.
 */
void setStringsTextController(struct TextConfig **configs)
{
    UBYTE i = 0;
    ctx.textConfigs = configs;
    while (ctx.textConfigs[i])
    {
        setStringTextController(ctx.textConfigs[i]);
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
    ctx.pauseCounter = 0;
    c->charIndex = 0;
    memset(c->characters, 0, sizeof(c->characters));
    c->currentChar = 0;

    // analyse first character in text string
    getCharData(c->currentText[c->currentChar], &CURRENT_CHAR(c));
    CURRENT_CHAR(c).xPos = ctx.scrollControlWidth - CURRENT_CHAR(c).xSize;
    CURRENT_CHAR(c).yPos = c->charYPosDestination;

    // save background at character starting position
    CURRENT_CHAR(c).oldBackground = AllocBitMap(MAX_CHAR_WIDTH, MAX_CHAR_HEIGHT,
                                                ctx.charDepth, BMF_CLEAR, NULL);
    saveCharacterBackground(c);
}

/**
 * Execute text scroller engine. Should be called
 * for each new frame.
 */
void executeTextController()
{
    UBYTE i = 0;
    while (ctx.textConfigs[i])
    {
        switch (ctx.textConfigs[i]->currentState)
        {
        case TC_SCROLL_IN:
            textScrollIn(ctx.textConfigs[i]);
            break;
        case TC_SCROLL_PAUSE:
            textScrollPause(ctx.textConfigs[i]);
            break;
        case TC_SCROLL_OUT:
            textScrollOut(ctx.textConfigs[i]);
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
    ctx.pauseTime = newPauseTime;
}

void textScrollPause(struct TextConfig *textConfig)
{
    if (ctx.pauseCounter >= ctx.pauseTime)
    {
        textConfig->currentState = TC_SCROLL_OUT;
        return;
    }

    ctx.pauseCounter++;
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
    while (ctx.textConfigs[i])
    {
        if (ctx.textConfigs[i]->currentState != TC_SCROLL_FINISHED)
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
    CURRENT_CHAR(c).oldBackground = AllocBitMap(MAX_CHAR_WIDTH, MAX_CHAR_HEIGHT,
                                                ctx.charDepth, BMF_CLEAR, NULL);
    CURRENT_CHAR(c).xPos = ctx.scrollControlWidth - CURRENT_CHAR(c).xSize;
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
    while (ctx.textConfigs[i])
    {
        resetTextConfig(ctx.textConfigs[i]);
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
    BltBitMap(ctx.fontBlob,
              CURRENT_CHAR(c).xPosInFont, CURRENT_CHAR(c).yPosInFont,
              ctx.textDestination,
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
              0, 0, ctx.textDestination,
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
    BltBitMap(ctx.textDestination, CURRENT_CHAR(c).xPos,
              CURRENT_CHAR(c).yPos,
              CURRENT_CHAR(c).oldBackground, 0, 0,
              CURRENT_CHAR(c).xSize,
              CURRENT_CHAR(c).ySize, 0xC0,
              0xff, 0);
}

/**
 * This data highly depends on the font
 */
void getCharData(char letter, struct CharBlob *charBlob)
{
    letter = tolower(letter);

    switch (letter)
    {
    case 'a':
        charBlob->xSize = 28;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 1;
        charBlob->yPosInFont = 0;
        break;
    case 'b':
        charBlob->xSize = 21;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 41;
        charBlob->yPosInFont = 0;
        break;
    case 'c':
        charBlob->xSize = 22;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 81;
        charBlob->yPosInFont = 0;
        break;
    case 'd':
        charBlob->xSize = 25;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 121;
        charBlob->yPosInFont = 0;
        break;
    case 'e':
        charBlob->xSize = 19;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 161;
        charBlob->yPosInFont = 0;
        break;
    case 'f':
        charBlob->xSize = 19;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 201;
        charBlob->yPosInFont = 0;
        break;
    case 'g':
        charBlob->xSize = 24;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 241;
        charBlob->yPosInFont = 0;
        break;
    case 'h':
        charBlob->xSize = 21;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 1;
        charBlob->yPosInFont = 40;
        break;
    case 'i':
        charBlob->xSize = 5;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 41;
        charBlob->yPosInFont = 40;
        break;
    case 'j':
        charBlob->xSize = 19;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 81;
        charBlob->yPosInFont = 40;
        break;
    case 'k':
        charBlob->xSize = 18;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 121;
        charBlob->yPosInFont = 40;
        break;
    case 'l':
        charBlob->xSize = 19;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 161;
        charBlob->yPosInFont = 40;
        break;
    case 'm':
        charBlob->xSize = 27;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 201;
        charBlob->yPosInFont = 40;
        break;
    case 'n':
        charBlob->xSize = 21;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 241;
        charBlob->yPosInFont = 40;
        break;
    case 'o':
        charBlob->xSize = 27;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 1;
        charBlob->yPosInFont = 80;
        break;
    case 'p':
        charBlob->xSize = 23;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 41;
        charBlob->yPosInFont = 80;
        break;
    case 'q':
        charBlob->xSize = 30;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 81;
        charBlob->yPosInFont = 80;
        break;
    case 'r':
        charBlob->xSize = 23;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 121;
        charBlob->yPosInFont = 80;
        break;
    case 's':
        charBlob->xSize = 22;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 161;
        charBlob->yPosInFont = 80;
        break;
    case 't':
        charBlob->xSize = 27;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 201;
        charBlob->yPosInFont = 80;
        break;
    case 'u':
        charBlob->xSize = 21;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 241;
        charBlob->yPosInFont = 80;
        break;
    case 'v':
        charBlob->xSize = 25;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 1;
        charBlob->yPosInFont = 120;
        break;
    case 'w':
        charBlob->xSize = 27;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 41;
        charBlob->yPosInFont = 120;
        break;
    case 'x':
        charBlob->xSize = 25;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 81;
        charBlob->yPosInFont = 120;
        break;
    case 'y':
        charBlob->xSize = 26;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 121;
        charBlob->yPosInFont = 120;
        break;
    case 'z':
        charBlob->xSize = 20;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 161;
        charBlob->yPosInFont = 120;
        break;
    case ' ':
        charBlob->xSize = 12;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 201;
        charBlob->yPosInFont = 120;
        break;
    default:
        charBlob->xSize = 12;
        charBlob->ySize = 33;
        charBlob->xPosInFont = 201;
        charBlob->yPosInFont = 120;
        break;
    }
}
