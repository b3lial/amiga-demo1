/*
 * textcontroller.h
 *
 *  Created on: Jan 16, 2021
 *      Author: belial
 */

#ifndef TEXTCONTROLLER_H_
#define TEXTCONTROLLER_H_

#include <exec/types.h>

struct FontInfo{
	UWORD xSize;
	UWORD ySize;
	UWORD characterPosInFontX;
	UWORD characterPosInFontY;
};

void initTextScrollEngine(char *text, UWORD firstX, UWORD firstY,
		UWORD depth);
void executeTextScrollEngine();
void terminateTextScrollEngine();

void getCharData(char letter, struct FontInfo* fontInfo);
void displayText(char *text, WORD xPos, WORD yPos);
void displayCharacter(char letter, WORD *xPos, WORD *yPos);

#endif /* TEXTCONTROLLER_H_ */
