#include "textscroller.h"

#include <stdio.h>
#include <ctype.h>

#include <exec/types.h>
#include <proto/graphics.h>
#include <proto/exec.h>

#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <graphics/videocontrol.h>
#include <dos/dos.h>

#include "starlight/starlight.h"
#include "main.h"

WORD payloadTextScrollerState = VIEW_TEXTSCROLLER_INIT;
struct BitMap *fontBlob = NULL;
struct BitMap *textscrollerScreen = NULL;

WORD fsmTextScroller(void) {
	switch (payloadTextScrollerState) {
	case VIEW_TEXTSCROLLER_INIT:
		initTextScroller();
		payloadTextScrollerState = VIEW_TEXTSCROLLER_RUNNING;
		break;

	case VIEW_TEXTSCROLLER_RUNNING:
		if (!executeTextScroller()) {
			payloadTextScrollerState = VIEW_TEXTSCROLLER_SHUTDOWN;
		}
		break;

	case VIEW_TEXTSCROLLER_SHUTDOWN:
		exitTextScroller();
		return MODULE_FINISHED;
	}

	return MODULE_CONTINUE;
}

void initTextScroller(void) {
	UWORD colortable0[] = { BLACK, RED, GREEN, BLUE, BLACK, RED, GREEN, BLUE };
	BYTE i = 0;
	writeLog("\n== Initialize View: TextScroller ==\n");

	//Load Charset Sprite and its Colors
	fontBlob = loadBlob("img/charset_final.RAW", VIEW_TEXTSCROLLER_DEPTH,
	VIEW_TEXTSCROLLER_FONT_WIDTH, VIEW_TEXTSCROLLER_FONT_HEIGHT);
	if (fontBlob == NULL) {
		writeLog("Error: Payload TextScroller, could not load font blob\n");
		exitTextScroller();
		exitSystem(RETURN_ERROR);
	}
	writeLogFS(
			"TextScroller BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
			fontBlob->BytesPerRow, fontBlob->Rows, fontBlob->Flags,
			fontBlob->pad);
	loadColorMap("img/charset_final.CMAP", colortable0,
			VIEW_TEXTSCROLLER_COLORS);

	//Create View and ViewExtra memory structures
	initView();

	//Create Bitmap for ViewPort
	textscrollerScreen = createBitMap(VIEW_TEXTSCROLLER_DEPTH,
			VIEW_TEXTSCROLLER_WIDTH,
			VIEW_TEXTSCROLLER_HEIGHT);
	for (i = 0; i < VIEW_TEXTSCROLLER_DEPTH; i++) {
		BltClear(textscrollerScreen->Planes[i],
				(textscrollerScreen->BytesPerRow) * (textscrollerScreen->Rows),
				1);
	}
	writeLogFS("Screen BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
			textscrollerScreen->BytesPerRow, textscrollerScreen->Rows,
			textscrollerScreen->Flags, textscrollerScreen->pad);

	//Add previously created BitMap to ViewPort so its shown on Screen
	addViewPort(textscrollerScreen, NULL, colortable0, VIEW_TEXTSCROLLER_COLORS,
			0, 0, VIEW_TEXTSCROLLER_WIDTH, VIEW_TEXTSCROLLER_HEIGHT);

	//Copy Text into ViewPort
	displayText("hi curly", 10, 10);

	//Make View visible
	startView();
}

BOOL executeTextScroller(void) {
	if (mouseClick()) {
		return FALSE;
	} else {
		return TRUE;
	}
}

void exitTextScroller(void) {
	stopView();
	cleanBitMap(textscrollerScreen);
	cleanBitMap(fontBlob);
}

/**
 * Display text on screen using font provided in src bitmap
 */
void displayText(char *text, WORD xPos, WORD yPos) {
	BYTE len = strlen(text);
	BYTE i;

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
		case 'a': xSize = 39; ySize = 33; characterPosInFontX = 1; characterPosInFontY = 0; break;
		case 'b': xSize = 39; ySize = 33; characterPosInFontX = 41; characterPosInFontY = 0; break;
		case 'c': xSize = 39; ySize = 33; characterPosInFontX = 81; characterPosInFontY = 0; break;
		case 'd': xSize = 39; ySize = 33; characterPosInFontX = 121; characterPosInFontY = 0; break;
		case 'e': xSize = 39; ySize = 33; characterPosInFontX = 161; characterPosInFontY = 0; break;
		case 'f': xSize = 39; ySize = 33; characterPosInFontX = 201; characterPosInFontY = 0; break;
		case 'g': xSize = 39; ySize = 33; characterPosInFontX = 241; characterPosInFontY = 0; break;
		case 'h': xSize = 39; ySize = 33; characterPosInFontX = 1; characterPosInFontY = 40; break;
		case 'i': xSize = 39; ySize = 33; characterPosInFontX = 41; characterPosInFontY = 40; break;
		case 'j': xSize = 39; ySize = 33; characterPosInFontX = 81; characterPosInFontY = 40; break;
		case 'k': xSize = 39; ySize = 33; characterPosInFontX = 121; characterPosInFontY = 40; break;
		case 'l': xSize = 39; ySize = 33; characterPosInFontX = 161; characterPosInFontY = 40; break;
		case 'm': xSize = 39; ySize = 33; characterPosInFontX = 201; characterPosInFontY = 40; break;
		case 'n': xSize = 39; ySize = 33; characterPosInFontX = 241; characterPosInFontY = 40; break;
		case 'o': xSize = 39; ySize = 33; characterPosInFontX = 1; characterPosInFontY = 80; break;
		case 'p': xSize = 39; ySize = 33; characterPosInFontX = 41; characterPosInFontY = 80; break;
		case 'q': xSize = 39; ySize = 33; characterPosInFontX = 81; characterPosInFontY = 80; break;
		case 'r': xSize = 39; ySize = 33; characterPosInFontX = 121; characterPosInFontY = 80; break;
		case 's': xSize = 39; ySize = 33; characterPosInFontX = 161; characterPosInFontY = 80; break;
		case 't': xSize = 39; ySize = 33; characterPosInFontX = 201; characterPosInFontY = 80; break;
		case 'u': xSize = 39; ySize = 33; characterPosInFontX = 241; characterPosInFontY = 80; break;
		case 'v': xSize = 39; ySize = 33; characterPosInFontX = 1; characterPosInFontY = 12; break;
		case 'w': xSize = 39; ySize = 33; characterPosInFontX = 41; characterPosInFontY = 120; break;
		case 'x': xSize = 39; ySize = 33; characterPosInFontX = 81; characterPosInFontY = 120; break;
		case 'y': xSize = 39; ySize = 33; characterPosInFontX = 121; characterPosInFontY = 120; break;
		case 'z': xSize = 39; ySize = 33; characterPosInFontX = 201; characterPosInFontY = 120; break;
		case ' ': xSize = 39; ySize = 33; characterPosInFontX = 241; characterPosInFontY = 120; break;
		default: return;
	}

	writeLogFS("displayCharacter: letter %c in font(%d,%d) to display(%d,%d)\n",
			letter, characterPosInFontX, characterPosInFontY, *xPos, *yPos);

	BltBitMap(fontBlob, characterPosInFontX, characterPosInFontY,
			textscrollerScreen, *xPos, *yPos, xSize, ySize, 0xC0, 0xff, 0);
	*xPos += xSize;
}
