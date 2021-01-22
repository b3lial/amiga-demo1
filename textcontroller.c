#include "textcontroller.h"
#include "starlight/starlight.h"

#include <graphics/gfxbase.h>

#include <stdio.h>
#include <ctype.h>
#include <string.h>

extern struct BitMap *fontBlob;
extern struct BitMap *textscrollerScreen;

/*
 * Vorgehen:
 *
 * execute() methode im vsync ausführen
 * hat zeiger auf aktuelles char und dessen gewünschte position
 *
 * loop über char array:
 * - variablen: zeiger auf aktuelles zeichen, dessen zielposition, dessen aktuelle position
 * - lösche letztes zeichen, außer wenn gerade neues zeichen beginnt
 * - zeichne zeichen bei x+1
 * - gucke ob zeichen ziel erreicht hat, wenn ja setze zeiger auf nächstes zeichen
 *   -> berechne dessen zielposition
 */

UWORD firstCharXPos = 0;
UWORD firstCharYPos = 0;
UWORD currentCharPosX = 0;
UWORD currentCharPosY = 0;
char *currentText = NULL;
UWORD currentChar = 0;
struct BitMap *previousCharacterData = NULL;

UWORD scrollControlWidth = 0;
UWORD scrollControlHeight = 0;

void initTextScrollEngine(char *text, UWORD firstX, UWORD firstY,
		UWORD depth,UWORD screenWidth, UWORD screenHeight){
	struct FontInfo fontInfo;

	firstCharXPos = firstX;
	firstCharYPos = firstY;

	scrollControlWidth = screenWidth;
	scrollControlHeight = screenHeight;

	getCharData(text[0], &fontInfo);
	currentCharPosX = screenWidth - fontInfo.xSize;
	currentCharPosY = firstY;

	currentText = text;
	currentChar = 0;

	//50*50 is enough because biggest character is about 30*33
	previousCharacterData = createBitMap(depth, 50, 50);
	saveBackground(currentText[currentChar], currentCharPosX, currentCharPosY);
}

void executeTextScrollEngine(){
	if(currentCharPosX == firstCharXPos){
		return;
	}

	restoreBackground(currentText[currentChar], currentCharPosX, currentCharPosY);
	saveBackground(currentText[currentChar], currentCharPosX-1, currentCharPosY);
	displayCharacter(currentText[currentChar], currentCharPosX, currentCharPosY);
	currentCharPosX--;
}

void terminateTextScrollEngine(){
	cleanBitMap(previousCharacterData);
}

/**
 * Save part of background bitmap in previousCharacterData
 */
void saveBackground(char letter, UWORD xPos, UWORD yPos) {
	struct FontInfo fontInfo;
	getCharData(letter, &fontInfo);
	BltBitMap(textscrollerScreen, xPos, xPos, previousCharacterData, 0, 0,
			fontInfo.xSize, fontInfo.ySize, 0xC0, 0xff, 0);
}

/**
 * Restore part of background bitmap from previousCharacterData
 */
void restoreBackground(char letter, UWORD xPos, UWORD yPos){
	struct FontInfo fontInfo;
	getCharData(letter, &fontInfo);
	BltBitMap(previousCharacterData, 0,
			0, textscrollerScreen, xPos, yPos,
			fontInfo.xSize, fontInfo.ySize, 0xC0, 0xff, 0);
}

/**
 * Display text on screen using font provided in src bitmap
 */
void displayText(char *text, WORD xPos, WORD yPos) {
	BYTE len = strlen(text);
	BYTE i;

	writeLog("\n== displayText() ==\n");
	for (i = 0; i < len; i++) {
		char currentChar = tolower(text[i]);
		//ignore not supported characters
		if (currentChar != ' ' && (currentChar < 'a' || currentChar > 'z')) {
			writeLogFS("displayText: letter %s not supported, skipping\n",
					currentChar);
			continue;
		}

		//displayCharacter(currentChar, &xPos, &yPos);
	}
}

/**
 * Print a character on screen. Add size on xPos/yPos to have coordinates
 * for the next character.
 */
void displayCharacter(char letter, WORD xPos, WORD yPos) {
	struct FontInfo fontInfo;

	//get size and position in font of corresponding character
	getCharData(letter, &fontInfo);

	writeLogFS("displayCharacter: letter %c in font(%d,%d) to display(%d,%d)\n",
			letter, fontInfo.characterPosInFontX, fontInfo.characterPosInFontY,
			xPos, yPos);

	/*
	 * Don't erase background if character rectangle (B) is blitted into destination (C,D)
	 * Therefore, we use minterm: BC+NBC+BNC -> 1110xxxx -> 0xE0
	 */
	BltBitMap(fontBlob, fontInfo.characterPosInFontX,
			fontInfo.characterPosInFontY, textscrollerScreen, xPos, yPos,
			fontInfo.xSize, fontInfo.ySize, 0xE0, 0xff, 0);
	//*xPos += (fontInfo.xSize + 5);
}

/**
 * This data highly depends on the font
 */
void getCharData(char letter, struct FontInfo *fontInfo) {
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
