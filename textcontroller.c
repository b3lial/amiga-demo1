// Copyright 2021 Christian Ammann

#include "textcontroller.h"

#include <ctype.h>
#include <graphics/gfxbase.h>
#include <stdio.h>
#include <string.h>

#include "starlight/starlight.h"

struct BitMap *fontBlob;
struct BitMap *textDestination;
struct TextConfig** textConfigs;

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
void exitTextController(void){
    cleanBitMap(fontBlob);
}

/**
 * Must be called first to init the whole engine.
 * Afterwards, executeTextScroller() can be called.
 */
void setStringsTextController(struct TextConfig **configs){
    UBYTE i = 0;
    textConfigs = configs;
    while(textConfigs[i]){
        setStringTextController(textConfigs[i]);
        i++;
    }
}

/**
 * Is called for each element in text list
 */
void setStringTextController(struct TextConfig* c)
{
    // Init engines global variables
    c->currentState = TC_SCROLL_IN;
    pauseCounter = 0;
    c->charIndex = 0;
    memset(c->characters, 0, sizeof(c->characters));
    c->currentChar = 0;

    // analyse first character in text string
    getCharData(c->currentText[c->currentChar], &(c->characters[c->charIndex]));
    c->characters[c->charIndex].xPos =
        scrollControlWidth - c->characters[c->charIndex].xSize;
    c->characters[c->charIndex].yPos = c->charYPosDestination;

    // save background at character starting position
    c->characters[c->charIndex].oldBackground = createBitMap(charDepth, 50, 50);
    BltBitMap(textDestination, c->characters[c->charIndex].xPos,
              c->characters[c->charIndex].yPos,
              c->characters[c->charIndex].oldBackground, 0, 0,
              c->characters[c->charIndex].xSize,
              c->characters[c->charIndex].ySize, 0xC0,
              0xff, 0);
}

/**
 * Execute text scroller engine. Should be called
 * for each new frame.
 */
void executeTextController()
{
    UBYTE i = 0;
    while(textConfigs[i]){
        switch(textConfigs[i]->currentState){
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

void textScrollIn(struct TextConfig* textConfig)
{
    // check whether every char was moved at its position
    if (textConfig->currentText[textConfig->currentChar] == 0)
    {
        textConfig->maxCharIndex = textConfig->charIndex;
        textConfig->charIndex = 0;
        textConfig->charXPosDestination = 0;
        textConfig->currentState = TC_SCROLL_PAUSE;
        return;
    }

    // restore previously saved background and character position
    BltBitMap(textConfig->characters[textConfig->charIndex].oldBackground, 0, 0,
              textDestination, textConfig->characters[textConfig->charIndex].xPos,
              textConfig->characters[textConfig->charIndex].yPos,
              textConfig->characters[textConfig->charIndex].xSize,
              textConfig->characters[textConfig->charIndex].ySize, 0xC0, 0xff, 0);

    // move character to next position
    textConfig->characters[textConfig->charIndex].xPos -= TEXT_MOVEMENT_SPEED;
    if (textConfig->characters[textConfig->charIndex].xPos < textConfig->charXPosDestination)
    {
        textConfig->characters[textConfig->charIndex].xPos = textConfig->charXPosDestination;
    }

    // save background there
    BltBitMap(textDestination,
              textConfig->characters[textConfig->charIndex].xPos,
              textConfig->characters[textConfig->charIndex].yPos,
              textConfig->characters[textConfig->charIndex].oldBackground, 0, 0,
              textConfig->characters[textConfig->charIndex].xSize,
              textConfig->characters[textConfig->charIndex].ySize, 0xC0,
              0xff, 0);

    // blit character on screen
    displayCurrentCharacter(textConfig);

    // finally, check whether we have to switch to next letter
    if (textConfig->characters[textConfig->charIndex].xPos <= textConfig->charXPosDestination)
    {
        prepareForNextCharacter(textConfig);
    }
}

void pauseTimeTextController(UWORD newPauseTime){
    pauseTime = newPauseTime;
}

void textScrollPause(struct TextConfig* textConfig){
    if(pauseCounter >= pauseTime){
        textConfig->currentState = TC_SCROLL_OUT;
        return;
    }

    pauseCounter++;
}

void textScrollOut(struct TextConfig* textConfig)
{
    // check whether every char was scrolled out and we are finished
    if (textConfig->charIndex > textConfig->maxCharIndex)
    {
        textConfig->currentState = TC_SCROLL_FINISHED;
        return;
    }

    // restore previously saved background and character position
    BltBitMap(textConfig->characters[textConfig->charIndex].oldBackground, 0, 0,
              textDestination, textConfig->characters[textConfig->charIndex].xPos,
              textConfig->characters[textConfig->charIndex].yPos,
              textConfig->characters[textConfig->charIndex].xSize,
              textConfig->characters[textConfig->charIndex].ySize, 0xC0, 0xff, 0);

    // char reached left side, delete it and switch to next char
    if (textConfig->characters[textConfig->charIndex].xPos == textConfig->charXPosDestination)
    {
        textConfig->charIndex++;
        return;
    }

    // move character to next position
    textConfig->characters[textConfig->charIndex].xPos -= TEXT_MOVEMENT_SPEED;
    if (textConfig->characters[textConfig->charIndex].xPos < 0)
    {
        textConfig->characters[textConfig->charIndex].xPos = 0;
    }

    // save background there
    BltBitMap(textDestination,
              textConfig->characters[textConfig->charIndex].xPos,
              textConfig->characters[textConfig->charIndex].yPos,
              textConfig->characters[textConfig->charIndex].oldBackground, 0, 0,
              textConfig->characters[textConfig->charIndex].xSize,
              textConfig->characters[textConfig->charIndex].ySize, 0xC0,
              0xff, 0);

    // blit character on screen
    displayCurrentCharacter(textConfig);
}

BOOL isFinishedTextController(void)
{
    BOOL allFinished = TRUE;
    UBYTE i = 0;
    while(textConfigs[i]){
        if(textConfigs[i]->currentState != TC_SCROLL_FINISHED){
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
void prepareForNextCharacter(struct TextConfig* textConfig)
{
    char letter = 0;
    textConfig->charXPosDestination += (textConfig->characters[textConfig->charIndex].xSize + 5);

    // skip space
    textConfig->currentChar++;
    letter = tolower(textConfig->currentText[textConfig->currentChar]);
    // reach end of string
    if (letter == 0)
    {
        return;
    }

    while (letter < 'a' || letter > 'z')
    {
        textConfig->charXPosDestination += 15;
        textConfig->currentChar++;
        letter = tolower(textConfig->currentText[textConfig->currentChar]);

        // reach end of string
        if (letter == 0)
        {
            return;
        }
    }

    // found next character, prepare everything for his arrival
    textConfig->charIndex++;
    getCharData(letter, &(textConfig->characters[textConfig->charIndex]));
    textConfig->characters[textConfig->charIndex].oldBackground = createBitMap(charDepth, 50, 50);
    textConfig->characters[textConfig->charIndex].xPos =
        scrollControlWidth - textConfig->characters[textConfig->charIndex].xSize;
    textConfig->characters[textConfig->charIndex].yPos = textConfig->charYPosDestination;

    // save background at character starting position
    BltBitMap(textDestination,
              textConfig->characters[textConfig->charIndex].xPos,
              textConfig->characters[textConfig->charIndex].yPos,
              textConfig->characters[textConfig->charIndex].oldBackground, 0, 0,
              textConfig->characters[textConfig->charIndex].xSize,
              textConfig->characters[textConfig->charIndex].ySize, 0xC0,
              0xff, 0);
}

/**
 * Free the character background backup bitmaps of all text configs
 */
void resetTextController(void){
    UBYTE i = 0;
    while(textConfigs[i]){
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
            cleanBitMap(textConfig->characters[i].oldBackground);
            textConfig->characters[i].oldBackground = NULL;
        }
    }
}

/**
 * Print a character on screen. Return position of next
 * character
 */
UWORD displayCurrentCharacter(struct TextConfig* textConfig)
{
    /*
	 * Don't erase background if character rectangle (B) is blitted into destination (C,D)
	 * Therefore, we use minterm: BC+NBC+BNC -> 1110xxxx -> 0xE0
	 */
    BltBitMap(fontBlob,
              textConfig->characters[textConfig->charIndex].xPosInFont,
              textConfig->characters[textConfig->charIndex].yPosInFont,
              textDestination, 
              textConfig->characters[textConfig->charIndex].xPos, 
              textConfig->characters[textConfig->charIndex].yPos,
              textConfig->characters[textConfig->charIndex].xSize,
              textConfig->characters[textConfig->charIndex].ySize, 0xC0, 0xff, 0);
    return (UWORD)(textConfig->characters[textConfig->charIndex].xPos
                   + textConfig->characters[textConfig->charIndex].xSize + 5);
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
