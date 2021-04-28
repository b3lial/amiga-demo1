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
UBYTE currentCharacterOnScreen;
struct FontInfo charactersOnScreen[MAX_CHAR_PER_LINE];

UWORD scrollControlWidth = 0;

/*
 * Calculate start positions of characters, initialize
 * their stop position, allocate memory for background
 * save/restore buffer
 */
void initTextScrollEngine(char *text, UWORD firstXPosDestination,
                          UWORD firstYPosDestination, UWORD depth, UWORD screenWidth) {
    // Init engines global variables
    currentCharacterOnScreen = 0;
    memset(charactersOnScreen, 0, sizeof(charactersOnScreen));

    charXPosDestination = firstXPosDestination;
    charYPosDestination = firstYPosDestination;
    scrollControlWidth = screenWidth;
    currentText = text;
    currentChar = 0;
    charDepth = depth;

    getCharData(currentText[currentChar], &(charactersOnScreen[currentCharacterOnScreen]));
    charactersOnScreen[currentCharacterOnScreen].xPos =
        scrollControlWidth - charactersOnScreen[currentCharacterOnScreen].xSize;
    charactersOnScreen[currentCharacterOnScreen].yPos = charYPosDestination;

    // save background at character starting position
    charactersOnScreen[currentCharacterOnScreen].oldBackground = createBitMap(charDepth, 50, 50);
    BltBitMap(textscrollerScreen, charactersOnScreen[currentCharacterOnScreen].xPos,
              charactersOnScreen[currentCharacterOnScreen].yPos,
              charactersOnScreen[currentCharacterOnScreen].oldBackground, 0, 0,
              charactersOnScreen[currentCharacterOnScreen].xSize,
              charactersOnScreen[currentCharacterOnScreen].ySize, 0xC0,
              0xff, 0);
}

void executeTextScrollEngine() {
    // check whether every char was moved at its position
    if (currentText[currentChar] == 0) {
        return;
    }

    // restore previously saved background and character position
    BltBitMap(charactersOnScreen[currentCharacterOnScreen].oldBackground, 0, 0,
              textscrollerScreen, charactersOnScreen[currentCharacterOnScreen].xPos,
              charactersOnScreen[currentCharacterOnScreen].yPos,
              charactersOnScreen[currentCharacterOnScreen].xSize,
              charactersOnScreen[currentCharacterOnScreen].ySize, 0xC0, 0xff, 0);

    // move character to next position
    charactersOnScreen[currentCharacterOnScreen].xPos -= TEXT_MOVEMENT_SPEED;

    // save background there
    BltBitMap(textscrollerScreen,
              charactersOnScreen[currentCharacterOnScreen].xPos,
              charactersOnScreen[currentCharacterOnScreen].yPos,
              charactersOnScreen[currentCharacterOnScreen].oldBackground, 0, 0,
              charactersOnScreen[currentCharacterOnScreen].xSize,
              charactersOnScreen[currentCharacterOnScreen].ySize, 0xC0,
              0xff, 0);

    // blit character on screen
    displayCurrentCharacter(charactersOnScreen[currentCharacterOnScreen].xPos,
                            charactersOnScreen[currentCharacterOnScreen].yPos);

    // finally, check whether we have to switch to next letter
    if (charactersOnScreen[currentCharacterOnScreen].xPos <= charXPosDestination) {
        prepareForNextCharacter();
    }
}

/*
 * Search in text string for next non-whitespace
 * character and initialize its destination position
 */
void prepareForNextCharacter() {
    char letter = 0;
    charXPosDestination += (charactersOnScreen[currentCharacterOnScreen].xSize + 5);

    // skip space
    currentChar++;
    letter = tolower(currentText[currentChar]);
    // reach end of string
    if (letter == 0) {
        return;
    }

    while (letter < 'a' || letter > 'z') {
        charXPosDestination += 15;
        currentChar++;
        letter = tolower(currentText[currentChar]);

        // reach end of string
        if (letter == 0) {
            return;
        }
    }

    // found next character, prepare everything for his arrival
    currentCharacterOnScreen++;
    getCharData(letter, &(charactersOnScreen[currentCharacterOnScreen]));
    charactersOnScreen[currentCharacterOnScreen].oldBackground = createBitMap(charDepth, 50, 50);
    charactersOnScreen[currentCharacterOnScreen].xPos =
        scrollControlWidth - charactersOnScreen[currentCharacterOnScreen].xSize;
    charactersOnScreen[currentCharacterOnScreen].yPos = charYPosDestination;

    // save background at character starting position
    BltBitMap(textscrollerScreen,
              charactersOnScreen[currentCharacterOnScreen].xPos,
              charactersOnScreen[currentCharacterOnScreen].yPos,
              charactersOnScreen[currentCharacterOnScreen].oldBackground, 0, 0,
              charactersOnScreen[currentCharacterOnScreen].xSize,
              charactersOnScreen[currentCharacterOnScreen].ySize, 0xC0,
              0xff, 0);
}

void terminateTextScrollEngine() {
    UBYTE i = 0;
    for (; i < MAX_CHAR_PER_LINE; i++) {
        if (charactersOnScreen[i].oldBackground) {
            cleanBitMap(charactersOnScreen[i].oldBackground);
            charactersOnScreen[i].oldBackground = NULL;
        }
    }
}

/**
 * Print a character on screen. Return position of next
 * character
 */
UWORD displayCurrentCharacter(WORD xPos, WORD yPos) {
    /*
	 * Don't erase background if character rectangle (B) is blitted into destination (C,D)
	 * Therefore, we use minterm: BC+NBC+BNC -> 1110xxxx -> 0xE0
	 */
    BltBitMap(fontBlob,
              charactersOnScreen[currentCharacterOnScreen].characterPosInFontX,
              charactersOnScreen[currentCharacterOnScreen].characterPosInFontY,
              textscrollerScreen, xPos, yPos,
              charactersOnScreen[currentCharacterOnScreen].xSize,
              charactersOnScreen[currentCharacterOnScreen].ySize, 0xC0, 0xff, 0);
    return (UWORD)(xPos + charactersOnScreen[currentCharacterOnScreen].xSize + 5);
}

/**
 * This data highly depends on the font
 */
void getCharData(char letter, struct FontInfo *fontInfo) {
    letter = tolower(letter);

    switch (letter) {
        case 'a':
            fontInfo->xSize = 28;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 1;
            fontInfo->characterPosInFontY = 0;
            break;
        case 'b':
            fontInfo->xSize = 21;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 41;
            fontInfo->characterPosInFontY = 0;
            break;
        case 'c':
            fontInfo->xSize = 22;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 81;
            fontInfo->characterPosInFontY = 0;
            break;
        case 'd':
            fontInfo->xSize = 25;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 121;
            fontInfo->characterPosInFontY = 0;
            break;
        case 'e':
            fontInfo->xSize = 19;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 161;
            fontInfo->characterPosInFontY = 0;
            break;
        case 'f':
            fontInfo->xSize = 19;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 201;
            fontInfo->characterPosInFontY = 0;
            break;
        case 'g':
            fontInfo->xSize = 24;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 241;
            fontInfo->characterPosInFontY = 0;
            break;
        case 'h':
            fontInfo->xSize = 21;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 1;
            fontInfo->characterPosInFontY = 40;
            break;
        case 'i':
            fontInfo->xSize = 5;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 41;
            fontInfo->characterPosInFontY = 40;
            break;
        case 'j':
            fontInfo->xSize = 19;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 81;
            fontInfo->characterPosInFontY = 40;
            break;
        case 'k':
            fontInfo->xSize = 18;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 121;
            fontInfo->characterPosInFontY = 40;
            break;
        case 'l':
            fontInfo->xSize = 19;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 161;
            fontInfo->characterPosInFontY = 40;
            break;
        case 'm':
            fontInfo->xSize = 27;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 201;
            fontInfo->characterPosInFontY = 40;
            break;
        case 'n':
            fontInfo->xSize = 21;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 241;
            fontInfo->characterPosInFontY = 40;
            break;
        case 'o':
            fontInfo->xSize = 27;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 1;
            fontInfo->characterPosInFontY = 80;
            break;
        case 'p':
            fontInfo->xSize = 23;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 41;
            fontInfo->characterPosInFontY = 80;
            break;
        case 'q':
            fontInfo->xSize = 30;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 81;
            fontInfo->characterPosInFontY = 80;
            break;
        case 'r':
            fontInfo->xSize = 23;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 121;
            fontInfo->characterPosInFontY = 80;
            break;
        case 's':
            fontInfo->xSize = 22;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 161;
            fontInfo->characterPosInFontY = 80;
            break;
        case 't':
            fontInfo->xSize = 27;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 201;
            fontInfo->characterPosInFontY = 80;
            break;
        case 'u':
            fontInfo->xSize = 21;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 241;
            fontInfo->characterPosInFontY = 80;
            break;
        case 'v':
            fontInfo->xSize = 25;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 1;
            fontInfo->characterPosInFontY = 120;
            break;
        case 'w':
            fontInfo->xSize = 27;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 41;
            fontInfo->characterPosInFontY = 120;
            break;
        case 'x':
            fontInfo->xSize = 25;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 81;
            fontInfo->characterPosInFontY = 120;
            break;
        case 'y':
            fontInfo->xSize = 26;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 121;
            fontInfo->characterPosInFontY = 120;
            break;
        case 'z':
            fontInfo->xSize = 20;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 161;
            fontInfo->characterPosInFontY = 120;
            break;
        case ' ':
            fontInfo->xSize = 12;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 201;
            fontInfo->characterPosInFontY = 120;
            break;
        default:
            fontInfo->xSize = 12;
            fontInfo->ySize = 33;
            fontInfo->characterPosInFontX = 201;
            fontInfo->characterPosInFontY = 120;
            break;
    }
}
