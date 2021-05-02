// Copyright 2021 Christian Ammann

#include "textcontroller.h"

#include <ctype.h>
#include <graphics/gfxbase.h>
#include <stdio.h>
#include <string.h>

#include "starlight/starlight.h"

extern struct BitMap *fontBlob;
extern struct BitMap *textscrollerScreen;

/*
 * Contains the text which is displayed and 
 * an index to current char
 */
char *currentText = NULL;
UWORD currentChar = 0;

// At which position do we want to move the current character
UWORD charXPosDestination = 0;
UWORD charYPosDestination = 0;

// Color depth of font blob
UWORD charDepth = 0;

// contains data of characters blitted on screen
UBYTE charIndex;
UBYTE maxCharIndex;
struct CharBlob characters[MAX_CHAR_PER_LINE];

// Screen width
UWORD scrollControlWidth = 0;

/*
 * When the effect starts, a sequence of characters is moved from
 * right to left on screen. When each char is at its position, they
 * scoll out from right to left.
 */
BOOL moveIn = TRUE;

// true if text has left the building
BOOL textScrollFinished = FALSE;

/**
 * Must be called first.
 * Store parameters in global variables, analyse first
 * character of input string, save background of first
 * blit destination. 
 * Afterwards, executeTextScroller() can be called.
 */
void initTextController(char *text, UWORD firstXPosDestination,
                        UWORD firstYPosDestination, UWORD depth, UWORD screenWidth)
{
    // Init engines global variables
    charIndex = 0;
    memset(characters, 0, sizeof(characters));

    charXPosDestination = firstXPosDestination;
    charYPosDestination = firstYPosDestination;
    scrollControlWidth = screenWidth;
    currentText = text;
    currentChar = 0;
    charDepth = depth;

    // analyse first character in text string
    getCharData(currentText[currentChar], &(characters[charIndex]));
    characters[charIndex].xPos =
        scrollControlWidth - characters[charIndex].xSize;
    characters[charIndex].yPos = charYPosDestination;

    // save background at character starting position
    characters[charIndex].oldBackground = createBitMap(charDepth, 50, 50);
    BltBitMap(textscrollerScreen, characters[charIndex].xPos,
              characters[charIndex].yPos,
              characters[charIndex].oldBackground, 0, 0,
              characters[charIndex].xSize,
              characters[charIndex].ySize, 0xC0,
              0xff, 0);

    moveIn = TRUE;
    textScrollFinished = FALSE;
}

/**
 * Execute text scroller engine. Should be called
 * for each new frame.
 */
void executeTextController()
{
    moveIn ? textScrollIn() : textScrollOut();
}

void textScrollIn()
{
    // check whether every char was moved at its position
    if (currentText[currentChar] == 0)
    {
        maxCharIndex = charIndex;
        charIndex = 0;
        charXPosDestination = 0;
        moveIn = FALSE;
        return;
    }

    // restore previously saved background and character position
    BltBitMap(characters[charIndex].oldBackground, 0, 0,
              textscrollerScreen, characters[charIndex].xPos,
              characters[charIndex].yPos,
              characters[charIndex].xSize,
              characters[charIndex].ySize, 0xC0, 0xff, 0);

    // move character to next position
    characters[charIndex].xPos -= TEXT_MOVEMENT_SPEED;

    // save background there
    BltBitMap(textscrollerScreen,
              characters[charIndex].xPos,
              characters[charIndex].yPos,
              characters[charIndex].oldBackground, 0, 0,
              characters[charIndex].xSize,
              characters[charIndex].ySize, 0xC0,
              0xff, 0);

    // blit character on screen
    displayCurrentCharacter(characters[charIndex].xPos,
                            characters[charIndex].yPos);

    // finally, check whether we have to switch to next letter
    if (characters[charIndex].xPos <= charXPosDestination)
    {
        prepareForNextCharacter();
    }
}

void textScrollOut()
{
    // check whether every char was scrolled out and we are finished
    if (charIndex > maxCharIndex)
    {
        textScrollFinished = TRUE;
        return;
    }

    // restore previously saved background and character position
    BltBitMap(characters[charIndex].oldBackground, 0, 0,
              textscrollerScreen, characters[charIndex].xPos,
              characters[charIndex].yPos,
              characters[charIndex].xSize,
              characters[charIndex].ySize, 0xC0, 0xff, 0);

    // char reached left side, delete it and switch to next char
    if (characters[charIndex].xPos == charXPosDestination)
    {
        charIndex++;
        return;
    }

    // move character to next position
    characters[charIndex].xPos -= TEXT_MOVEMENT_SPEED;
    if (characters[charIndex].xPos < 0)
    {
        characters[charIndex].xPos = 0;
    }

    // save background there
    BltBitMap(textscrollerScreen,
              characters[charIndex].xPos,
              characters[charIndex].yPos,
              characters[charIndex].oldBackground, 0, 0,
              characters[charIndex].xSize,
              characters[charIndex].ySize, 0xC0,
              0xff, 0);

    // blit character on screen
    displayCurrentCharacter(characters[charIndex].xPos,
                            characters[charIndex].yPos);
}

BOOL isFinishedTextController(void)
{
    return textScrollFinished;
}

/*
 * Search in text string for next non-whitespace
 * character and initialize its destination position
 */
void prepareForNextCharacter()
{
    char letter = 0;
    charXPosDestination += (characters[charIndex].xSize + 5);

    // skip space
    currentChar++;
    letter = tolower(currentText[currentChar]);
    // reach end of string
    if (letter == 0)
    {
        return;
    }

    while (letter < 'a' || letter > 'z')
    {
        charXPosDestination += 15;
        currentChar++;
        letter = tolower(currentText[currentChar]);

        // reach end of string
        if (letter == 0)
        {
            return;
        }
    }

    // found next character, prepare everything for his arrival
    charIndex++;
    getCharData(letter, &(characters[charIndex]));
    characters[charIndex].oldBackground = createBitMap(charDepth, 50, 50);
    characters[charIndex].xPos =
        scrollControlWidth - characters[charIndex].xSize;
    characters[charIndex].yPos = charYPosDestination;

    // save background at character starting position
    BltBitMap(textscrollerScreen,
              characters[charIndex].xPos,
              characters[charIndex].yPos,
              characters[charIndex].oldBackground, 0, 0,
              characters[charIndex].xSize,
              characters[charIndex].ySize, 0xC0,
              0xff, 0);
}

/**
 * Free the character background backup bitmaps 
 */
void terminateTextController()
{
    UBYTE i = 0;
    for (; i < MAX_CHAR_PER_LINE; i++)
    {
        if (characters[i].oldBackground)
        {
            cleanBitMap(characters[i].oldBackground);
            characters[i].oldBackground = NULL;
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
              characters[charIndex].xPosInFont,
              characters[charIndex].yPosInFont,
              textscrollerScreen, xPos, yPos,
              characters[charIndex].xSize,
              characters[charIndex].ySize, 0xC0, 0xff, 0);
    return (UWORD)(xPos + characters[charIndex].xSize + 5);
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
