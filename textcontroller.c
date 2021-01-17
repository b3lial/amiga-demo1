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

UWORD firstCharXPos;
UWORD firstCharYPos;
UWORD currentCharPosX;
UWORD currentCharPosY;
char *currentText;
UWORD currentChar;

void initTextScrollEngine(char *text, UWORD firstX, UWORD firstY){
	firstCharXPos = firstX;
	firstCharYPos = firstY;
	currentCharPosX = 0;
	currentCharPosY = 0;
	currentText = text;
	currentChar = 0;
}

void executeTextScrollEngine(){

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

		displayCharacter(currentChar, &xPos, &yPos);
	}
}

/**
 * Print a character on screen. Add size on xPos/yPos to have coordinates
 * for the next character.
 */
void displayCharacter(char letter, WORD *xPos, WORD *yPos) {
	WORD xSize, ySize, characterPosInFontX, characterPosInFontY;

	//get size and position in font of corresponding character
	switch(letter){
		case 'a': xSize = 28; ySize = 33; characterPosInFontX = 1; characterPosInFontY = 0; break;
		case 'b': xSize = 21; ySize = 33; characterPosInFontX = 41; characterPosInFontY = 0; break;
		case 'c': xSize = 22; ySize = 33; characterPosInFontX = 81; characterPosInFontY = 0; break;
		case 'd': xSize = 25; ySize = 33; characterPosInFontX = 121; characterPosInFontY = 0; break;
		case 'e': xSize = 19; ySize = 33; characterPosInFontX = 161; characterPosInFontY = 0; break;
		case 'f': xSize = 19; ySize = 33; characterPosInFontX = 201; characterPosInFontY = 0; break;
		case 'g': xSize = 24; ySize = 33; characterPosInFontX = 241; characterPosInFontY = 0; break;
		case 'h': xSize = 21; ySize = 33; characterPosInFontX = 1; characterPosInFontY = 40; break;
		case 'i': xSize = 5; ySize = 33; characterPosInFontX = 41; characterPosInFontY = 40; break;
		case 'j': xSize = 19; ySize = 33; characterPosInFontX = 81; characterPosInFontY = 40; break;
		case 'k': xSize = 18; ySize = 33; characterPosInFontX = 121; characterPosInFontY = 40; break;
		case 'l': xSize = 19; ySize = 33; characterPosInFontX = 161; characterPosInFontY = 40; break;
		case 'm': xSize = 27; ySize = 33; characterPosInFontX = 201; characterPosInFontY = 40; break;
		case 'n': xSize = 21; ySize = 33; characterPosInFontX = 241; characterPosInFontY = 40; break;
		case 'o': xSize = 27; ySize = 33; characterPosInFontX = 1; characterPosInFontY = 80; break;
		case 'p': xSize = 23; ySize = 33; characterPosInFontX = 41; characterPosInFontY = 80; break;
		case 'q': xSize = 30; ySize = 33; characterPosInFontX = 81; characterPosInFontY = 80; break;
		case 'r': xSize = 23; ySize = 33; characterPosInFontX = 121; characterPosInFontY = 80; break;
		case 's': xSize = 22; ySize = 33; characterPosInFontX = 161; characterPosInFontY = 80; break;
		case 't': xSize = 27; ySize = 33; characterPosInFontX = 201; characterPosInFontY = 80; break;
		case 'u': xSize = 21; ySize = 33; characterPosInFontX = 241; characterPosInFontY = 80; break;
		case 'v': xSize = 25; ySize = 33; characterPosInFontX = 1; characterPosInFontY = 120; break;
		case 'w': xSize = 27; ySize = 33; characterPosInFontX = 41; characterPosInFontY = 120; break;
		case 'x': xSize = 25; ySize = 33; characterPosInFontX = 81; characterPosInFontY = 120; break;
		case 'y': xSize = 26; ySize = 33; characterPosInFontX = 121; characterPosInFontY = 120; break;
		case 'z': xSize = 20; ySize = 33; characterPosInFontX = 161; characterPosInFontY = 120; break;
		case ' ': xSize = 12; ySize = 33; characterPosInFontX = 201; characterPosInFontY = 120; break;
		default: return;
	}

	writeLogFS("displayCharacter: letter %c in font(%d,%d) to display(%d,%d)\n",
			letter, characterPosInFontX, characterPosInFontY, *xPos, *yPos);

	/*
	 * Don't erase background if character rectangle (B) is blitted into destination (C,D)
	 * Therefore, we use minterm: BC+NBC+BNC -> 1110xxxx -> 0xE0
	 */
	BltBitMap(fontBlob, characterPosInFontX, characterPosInFontY,
			textscrollerScreen, *xPos, *yPos, xSize, ySize, 0xE0, 0xff, 0);
	*xPos += (xSize + 5);
}
