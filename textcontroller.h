/*
 * textcontroller.h
 *
 *  Created on: Jan 16, 2021
 *      Author: belial
 */

#ifndef TEXTCONTROLLER_H_
#define TEXTCONTROLLER_H_

#include <exec/types.h>

void displayText(char *text, WORD xPos, WORD yPos);
void displayCharacter(char letter, WORD *xPos, WORD *yPos);

#endif /* TEXTCONTROLLER_H_ */
