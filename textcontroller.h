/*
 * textcontroller.h
 *
 *  Created on: Jan 16, 2021
 *      Author: belial
 */

#ifndef TEXTCONTROLLER_H_
#define TEXTCONTROLLER_H_

#include <exec/types.h>

#define TEXT_MOVEMENT_SPEED 3

struct FontInfo{
	UWORD xSize;
	UWORD ySize;
	UWORD characterPosInFontX;
	UWORD characterPosInFontY;
};

void initTextScrollEngine(char *text, UWORD firstX, UWORD firstY,
		UWORD depth, UWORD screenWidth);
void executeTextScrollEngine(void);
void terminateTextScrollEngine(void);

void getCharData(char letter, struct FontInfo* fontInfo);
void displayText(char *text, WORD xPos, WORD yPos);
UWORD displayCharacter(char letter, WORD xPos, WORD yPos);
void prepareForNextCharacter(char letter);

#endif /* TEXTCONTROLLER_H_ */
