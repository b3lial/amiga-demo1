// Copyright 2021 Christian Ammann

#include "textcontroller.h"

#include <ctype.h>
#include <graphics/gfxbase.h>
#include <stdio.h>
#include <string.h>

#include "starlight/starlight.h"

struct BitMap *fontBlob;
struct BitMap *textDestination;
struct TextConfig* textConfig;

// Color depth of font blob
UWORD charDepth = 0;

// Screen width
UWORD scrollControlWidth = 0;

UWORD pauseCounter = 0;

/**
 * Load font
 */
BOOL initTextController(struct BitMap *screen, UWORD depth, UWORD screenWidth)
{
    charDepth = depth;
    scrollControlWidth = screenWidth;
    textDestination = screen;

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
 * Must be called first.
 * Store parameters in global variables, analyse first
 * character of input string, save background of first
 * blit destination. 
 * Afterwards, executeTextScroller() can be called.
 */
void setStringTextController(struct TextConfig* config)
{
    textConfig = config;

    // Init engines global variables
    textConfig->currentState = TC_SCROLL_IN;
    pauseCounter = 0;
    textConfig->charIndex = 0;
    memset(textConfig->characters, 0, sizeof(textConfig->characters));
    textConfig->currentChar = 0;

    // analyse first character in text string
    getCharData(textConfig->currentText[textConfig->currentChar], &(textConfig->characters[textConfig->charIndex]));
    textConfig->characters[textConfig->charIndex].xPos =
        scrollControlWidth - textConfig->characters[textConfig->charIndex].xSize;
    textConfig->characters[textConfig->charIndex].yPos = textConfig->charYPosDestination;

    // save background at character starting position
    textConfig->characters[textConfig->charIndex].oldBackground = createBitMap(charDepth, 50, 50);
    BltBitMap(textDestination, textConfig->characters[textConfig->charIndex].xPos,
              textConfig->characters[textConfig->charIndex].yPos,
              textConfig->characters[textConfig->charIndex].oldBackground, 0, 0,
              textConfig->characters[textConfig->charIndex].xSize,
              textConfig->characters[textConfig->charIndex].ySize, 0xC0,
              0xff, 0);
}

/**
 * Execute text scroller engine. Should be called
 * for each new frame.
 */
void executeTextController()
{
    switch(textConfig->currentState){
        case TC_SCROLL_IN:
            textScrollIn();
            break;
        case TC_SCROLL_PAUSE:
            textScrollPause();
            break;
        case TC_SCROLL_OUT:
            textScrollOut();
            break;
        case TC_SCROLL_FINISHED:
            break;
    }
}

void textScrollIn()
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
    displayCurrentCharacter(textConfig->characters[textConfig->charIndex].xPos,
                            textConfig->characters[textConfig->charIndex].yPos);

    // finally, check whether we have to switch to next letter
    if (textConfig->characters[textConfig->charIndex].xPos <= textConfig->charXPosDestination)
    {
        prepareForNextCharacter();
    }
}

void textScrollPause(void){
    if(pauseCounter >= TEXT_PAUSE_TIME){
        textConfig->currentState = TC_SCROLL_OUT;
        return;
    }

    pauseCounter++;
}

void textScrollOut()
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
    displayCurrentCharacter(textConfig->characters[textConfig->charIndex].xPos,
                            textConfig->characters[textConfig->charIndex].yPos);
}

BOOL isFinishedTextController(void)
{
    return (textConfig->currentState == TC_SCROLL_FINISHED);
}

/*
 * Search in text string for next non-whitespace
 * character and initialize its destination position
 */
void prepareForNextCharacter()
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
 * Free the character background backup bitmaps 
 */
void resetTextController()
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
UWORD displayCurrentCharacter(WORD xPos, WORD yPos)
{
    /*
	 * Don't erase background if character rectangle (B) is blitted into destination (C,D)
	 * Therefore, we use minterm: BC+NBC+BNC -> 1110xxxx -> 0xE0
	 */
    BltBitMap(fontBlob,
              textConfig->characters[textConfig->charIndex].xPosInFont,
              textConfig->characters[textConfig->charIndex].yPosInFont,
              textDestination, xPos, yPos,
              textConfig->characters[textConfig->charIndex].xSize,
              textConfig->characters[textConfig->charIndex].ySize, 0xC0, 0xff, 0);
    return (UWORD)(xPos + textConfig->characters[textConfig->charIndex].xSize + 5);
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
