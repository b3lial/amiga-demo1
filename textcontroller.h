/*
 * textcontroller.h
 *
 *  Created on: Jan 16, 2021
 *      Author: belial
 */

#ifndef TEXTCONTROLLER_H_
#define TEXTCONTROLLER_H_

#include <exec/types.h>

#define TEXT_MOVEMENT_SPEED 2

struct FontInfo{
	UWORD xSize;
	UWORD ySize;
	UWORD characterPosInFontX;
	UWORD characterPosInFontY;
};

void initTextScrollEngine(char *text, UWORD firstX, UWORD firstY,
		UWORD depth, UWORD screenWidth, UWORD screenHeight);
void executeTextScrollEngine();
void terminateTextScrollEngine();

void saveBackground(char letter, UWORD xPos, UWORD yPos);
void restoreBackground(char letter, UWORD xPos, UWORD yPos);
void getCharData(char letter, struct FontInfo* fontInfo);
void displayText(char *text, WORD xPos, WORD yPos);
void displayCharacter(char letter, WORD xPos, WORD yPos);

#endif /* TEXTCONTROLLER_H_ */
