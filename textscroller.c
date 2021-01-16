#include "textscroller.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <exec/types.h>
#include <exec/memory.h>

#include <proto/graphics.h>
#include <proto/exec.h>

#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <graphics/videocontrol.h>

#include <dos/dos.h>

#include "starlight/starlight.h"
#include "main.h"

WORD payloadTextScrollerState = TEXTSCROLLER_INIT;
struct BitMap *fontBlob = NULL;
struct BitMap *spaceBlob = NULL;
struct BitMap *textscrollerScreen = NULL;
ULONG* colortable1 = NULL;

WORD fsmTextScroller(void) {
	switch (payloadTextScrollerState) {
	case TEXTSCROLLER_INIT:
		initTextScroller();
		payloadTextScrollerState = TEXTSCROLLER_RUNNING;
		break;

	case TEXTSCROLLER_RUNNING:
		if (!executeTextScroller()) {
			payloadTextScrollerState = TEXTSCROLLER_SHUTDOWN;
		}
		break;

	case TEXTSCROLLER_SHUTDOWN:
		exitTextScroller();
		return MODULE_FINISHED;
	}

	return MODULE_CONTINUE;
}

void initTextScroller(void) {
	UWORD colortable0[8];
	BYTE i = 0;
	writeLog("\n== initTextScroller() ==\n");

	//Load font bitmap and its colors
	writeLog("Load font bitmap and colors\n");
	fontBlob = loadBlob("img/charset_final.RAW", TEXTSCROLLER_BLOB_FONT_DEPTH,
	TEXTSCROLLER_BLOB_FONT_WIDTH, TEXTSCROLLER_BLOB_FONT_HEIGHT);
	if (fontBlob == NULL) {
		writeLog("Error: Could not load font blob\n");
		exitTextScroller();
		exitSystem(RETURN_ERROR);
	}
	writeLogFS(
			"Font BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
			fontBlob->BytesPerRow, fontBlob->Rows, fontBlob->Flags,
			fontBlob->pad);
	loadColorMap("img/charset_final.CMAP", colortable0,
			TEXTSCROLLER_BLOB_FONT_COLORS);

	//Load space background bitmap and colors
	writeLog("\nLoad space background bitmap and colors\n");
	spaceBlob = loadBlob("img/space3_320_148_8.RAW", TEXTSCROLLER_BLOB_SPACE_DEPTH,
	TEXTSCROLLER_BLOB_SPACE_WIDTH, TEXTSCROLLER_BLOB_SPACE_HEIGHT);
	if (spaceBlob == NULL) {
		writeLog("Error: Could not load space blob\n");
		exitTextScroller();
		exitSystem(RETURN_ERROR);
	}
	writeLogFS(
			"Space Bitmap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
			spaceBlob->BytesPerRow, spaceBlob->Rows, spaceBlob->Flags,
			spaceBlob->pad);

	//Load space background color table
	colortable1 = AllocVec(COLORMAP32_BYTE_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS), MEMF_ANY);
	if(!colortable1){
		writeLog("Error: Could not allocate memory for space bitmap color table\n");
		exitTextScroller();
		exitSystem(RETURN_ERROR);
	}
	writeLogFS("Allocated %d  bytes for space bitmap color table\n",
			COLORMAP32_BYTE_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS));
	loadColorMap32("img/space3_320_148_8.CMAP", colortable1, TEXTSCROLLER_BLOB_SPACE_COLORS);

	//Load Textscroller Screen Bitmap
	writeLog("\nLoad textscroller screen background bitmap\n");
	textscrollerScreen = createBitMap(TEXTSCROLLER_BLOB_FONT_DEPTH,
			TEXTSCROLLER_VIEW_WIDTH,
			TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT);
	for (i = 0; i < TEXTSCROLLER_BLOB_FONT_DEPTH; i++) {
		BltClear(textscrollerScreen->Planes[i],
				(textscrollerScreen->BytesPerRow) * (textscrollerScreen->Rows),
				1);
	}
	writeLogFS("TextScroller Screen BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
			textscrollerScreen->BytesPerRow, textscrollerScreen->Rows,
			textscrollerScreen->Flags, textscrollerScreen->pad);

	//Create View and ViewExtra memory structures
	writeLog("\nCreate view\n");
	initView();

	//Add previously created BitMap for text display to ViewPort so its shown on Screen
	addViewPort(textscrollerScreen, NULL, colortable0, TEXTSCROLLER_BLOB_FONT_COLORS, FALSE,
			0, 0, TEXTSCROLLER_VIEW_WIDTH, TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT);

	//Add space background BitMap to ViewPort so its shown on Screen
	addViewPort(spaceBlob, NULL, colortable1,
			COLORMAP32_LONG_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS), TRUE,
			0, TEXTSCROLLER_VIEW_TEXTSECTION_HEIGHT+2, TEXTSCROLLER_VIEW_WIDTH,
			TEXTSCROLLER_VIEW_SPACESECTION_HEIGHT);

	//clean the allocated memory of colortable, we dont need it anymore because we
	//have a proper copperlist now
	FreeVec(colortable1);
	writeLogFS("Freeing %d bytes of space bitmap color table\n",
			COLORMAP32_BYTE_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS));
	colortable1 = NULL;

	//Copy Text into ViewPort
	displayText("hi there", 70, 60);

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
	writeLog("\n== exitTextScroller() ==\n");

	if(colortable1){
		FreeVec(colortable1);
		colortable1 = NULL;
		writeLogFS("Freeing %d bytes of space bitmap color table\n",
				COLORMAP32_BYTE_SIZE(TEXTSCROLLER_BLOB_SPACE_COLORS));
	}
	stopView();
	cleanBitMap(textscrollerScreen);
	cleanBitMap(fontBlob);
	cleanBitMap(spaceBlob);
	payloadTextScrollerState = TEXTSCROLLER_INIT;
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
