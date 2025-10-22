#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <clib/graphics_protos.h>
#include <clib/exec_protos.h>
#include <ctype.h>
#include <string.h>

#include "textcontroller.h"

#include "utils/utils.h"
#include "gfx/graphicscontroller.h"

#define CURRENT_CHAR(STATE) (STATE->characters[STATE->charIndex])

// Private state struct - internal to text_controller
struct TextScrollState
{
    struct TextConfig *config;  // pointer to public config

    // Current character tracking
    UWORD currentChar;

    // Character position tracking
    UWORD charXPosDestination;
    UWORD charYPosDestination;

    // contains data of characters blitted on screen
    UBYTE charIndex;
    UBYTE maxCharIndex;
    struct CharBlob characters[MAX_CHAR_PER_LINE];

    // State machine state
    enum TEXT_CONTROL_STATE currentState;
};

struct TextControllerContext {
    struct BitMap *fontBlob;
    struct BitMap *textDestination;
    struct TextScrollState *scrollStates[TEXT_LIST_SIZE];
    UWORD charDepth;
    UWORD scrollControlWidth;
    UWORD pauseCounter;
    UWORD pauseTime;
};

static struct TextControllerContext ctx = {
    .fontBlob = NULL,
    .textDestination = NULL,
    .scrollStates = {NULL},
    .charDepth = 0,
    .scrollControlWidth = 0,
    .pauseCounter = 0,
    .pauseTime = 0
};

// Forward declarations of internal functions
static void resetTextController(void);
static void resetTextScrollState(struct TextScrollState *state);
static void setStringTextController(struct TextScrollState *state);
static void textScrollIn(struct TextScrollState *state);
static void textScrollPause(struct TextScrollState *state);
static void textScrollOut(struct TextScrollState *state);
static void getCharData(char letter, struct CharBlob *charBlob);
static UWORD displayCurrentCharacter(struct TextScrollState *state);
static void saveCharacterBackground(struct TextScrollState *state);
static void restorePreviousBackground(struct TextScrollState *state);
static void prepareForNextCharacter(struct TextScrollState *state);

//----------------------------------------
BOOL startTextController(struct BitMap *screen, UWORD depth, UWORD screenWidth)
{
    UBYTE i = 0;

    ctx.charDepth = depth;
    ctx.scrollControlWidth = screenWidth;
    ctx.textDestination = screen;
    ctx.pauseTime = TEXT_PAUSE_TIME;

    // Load font bitmap and its colors (only if not already loaded)
    if (ctx.fontBlob == NULL)
    {
        writeLog("Load font bitmap and colors\n");
        ctx.fontBlob = loadBlob("img/charset_final.RAW", ctx.charDepth,
                            TEXTSCROLLER_BLOB_FONT_WIDTH, TEXTSCROLLER_BLOB_FONT_HEIGHT);
        if (ctx.fontBlob == NULL)
        {
            writeLog("Error: Could not load font blob\n");
            goto _error_cleanup;
        }
        writeLogFS(
            "Font BitMap: BytesPerRow: %d, Rows: %d, Flags: %d, pad: %d\n",
            ctx.fontBlob->BytesPerRow, ctx.fontBlob->Rows, ctx.fontBlob->Flags,
            ctx.fontBlob->pad);
    }

    // Allocate scroll states and character background bitmaps upfront
    writeLog("Allocating scroll states and character background bitmaps\n");
    for (i = 0; i < TEXT_LIST_SIZE; i++)
    {
        UBYTE j = 0;

        ctx.scrollStates[i] = AllocVec(sizeof(struct TextScrollState), MEMF_ANY | MEMF_CLEAR);
        if (!ctx.scrollStates[i])
        {
            writeLog("Error: Could not allocate memory for scroll state\n");
            goto _error_cleanup;
        }

        // Pre-allocate background bitmaps for all possible characters
        for (j = 0; j < MAX_CHAR_PER_LINE; j++)
        {
            ctx.scrollStates[i]->characters[j].oldBackground =
                AllocBitMap(MAX_CHAR_WIDTH, MAX_CHAR_HEIGHT, ctx.charDepth, BMF_CLEAR, NULL);
            if (!ctx.scrollStates[i]->characters[j].oldBackground)
            {
                writeLog("Error: Could not allocate memory for character background bitmap\n");
                goto _error_cleanup;
            }
        }
    }

    return TRUE;

_error_cleanup:
    exitTextController();
    return FALSE;
}

//----------------------------------------
void exitTextController(void)
{
    UBYTE i = 0;

    // Free all scroll states and their character background bitmaps
    for (i = 0; i < TEXT_LIST_SIZE; i++)
    {
        if (ctx.scrollStates[i])
        {
            UBYTE j = 0;

            // Free character background bitmaps
            for (j = 0; j < MAX_CHAR_PER_LINE; j++)
            {
                if (ctx.scrollStates[i]->characters[j].oldBackground)
                {
                    FreeBitMap(ctx.scrollStates[i]->characters[j].oldBackground);
                    ctx.scrollStates[i]->characters[j].oldBackground = NULL;
                }
            }

            FreeVec(ctx.scrollStates[i]);
            ctx.scrollStates[i] = NULL;
        }
    }

    if (ctx.fontBlob)
    {
        FreeBitMap(ctx.fontBlob);
        ctx.fontBlob = NULL;
    }
}

//----------------------------------------
void configureTextController(struct TextConfig **configs, UWORD pauseTime)
{
    UBYTE i = 0;

    // Reset previous state first
    resetTextController();

    // Set pause time (use default if 0)
    ctx.pauseTime = pauseTime ? pauseTime : TEXT_PAUSE_TIME;

    // Initialize scroll states with configs
    while (configs[i] && i < TEXT_LIST_SIZE)
    {
        ctx.scrollStates[i]->config = configs[i];
        ctx.scrollStates[i]->charXPosDestination = configs[i]->charXPosDestination;
        ctx.scrollStates[i]->charYPosDestination = configs[i]->charYPosDestination;

        setStringTextController(ctx.scrollStates[i]);
        i++;
    }

    // Mark end of active configs (remaining states stay allocated but unused)
    if (i < TEXT_LIST_SIZE)
    {
        ctx.scrollStates[i]->config = NULL;
    }
}

/**
 * Is called for each element in text list
 */
static void setStringTextController(struct TextScrollState *state)
{
    // Init engines global variables
    state->currentState = TC_SCROLL_IN;
    ctx.pauseCounter = 0;
    state->charIndex = 0;
    state->currentChar = 0;

    // analyse first character in text string
    getCharData(state->config->currentText[state->currentChar], &CURRENT_CHAR(state));
    CURRENT_CHAR(state).xPos = ctx.scrollControlWidth - CURRENT_CHAR(state).xSize;
    CURRENT_CHAR(state).yPos = state->charYPosDestination;

    // save background at character starting position
    saveCharacterBackground(state);
}

//----------------------------------------
void executeTextController()
{
    UBYTE i = 0;
    while (i < TEXT_LIST_SIZE && ctx.scrollStates[i]->config)
    {
        switch (ctx.scrollStates[i]->currentState)
        {
        case TC_SCROLL_IN:
            textScrollIn(ctx.scrollStates[i]);
            break;
        case TC_SCROLL_PAUSE:
            textScrollPause(ctx.scrollStates[i]);
            break;
        case TC_SCROLL_OUT:
            textScrollOut(ctx.scrollStates[i]);
            break;
        case TC_SCROLL_FINISHED:
            break;
        }
        i++;
    }
}

static void textScrollIn(struct TextScrollState *state)
{
    // check whether every char was moved at its position
    if (state->config->currentText[state->currentChar] == 0)
    {
        state->maxCharIndex = state->charIndex;
        state->charIndex = 0;
        state->charXPosDestination = 0;
        state->currentState = TC_SCROLL_PAUSE;
        return;
    }

    // restore previously saved background and character position
    restorePreviousBackground(state);

    // move character to next position
    CURRENT_CHAR(state).xPos -= TEXT_MOVEMENT_SPEED;
    if (CURRENT_CHAR(state).xPos < state->charXPosDestination)
    {
        CURRENT_CHAR(state).xPos = state->charXPosDestination;
    }

    // save background there
    saveCharacterBackground(state);

    // blit character on screen
    displayCurrentCharacter(state);

    // finally, check whether we have to switch to next letter
    if (CURRENT_CHAR(state).xPos <= state->charXPosDestination)
    {
        prepareForNextCharacter(state);
    }
}

static void textScrollPause(struct TextScrollState *state)
{
    if (ctx.pauseCounter >= ctx.pauseTime)
    {
        state->currentState = TC_SCROLL_OUT;
        return;
    }

    ctx.pauseCounter++;
}

static void textScrollOut(struct TextScrollState *state)
{
    // check whether every char was scrolled out and we are finished
    if (state->charIndex > state->maxCharIndex)
    {
        state->currentState = TC_SCROLL_FINISHED;
        return;
    }

    // restore previously saved background and character position
    restorePreviousBackground(state);

    // char reached left side, delete it and switch to next char
    if (CURRENT_CHAR(state).xPos == state->charXPosDestination)
    {
        state->charIndex++;
        return;
    }

    // move character to next position
    CURRENT_CHAR(state).xPos -= TEXT_MOVEMENT_SPEED;
    if (CURRENT_CHAR(state).xPos < 0)
    {
        CURRENT_CHAR(state).xPos = 0;
    }

    // save background there
    saveCharacterBackground(state);

    // blit character on screen
    displayCurrentCharacter(state);
}

//----------------------------------------
BOOL isFinishedTextController(void)
{
    BOOL allFinished = TRUE;
    UBYTE i = 0;
    while (i < TEXT_LIST_SIZE && ctx.scrollStates[i]->config)
    {
        if (ctx.scrollStates[i]->currentState != TC_SCROLL_FINISHED)
        {
            allFinished = FALSE;
        }
        i++;
    }
    return allFinished;
}

//----------------------------------------
static void prepareForNextCharacter(struct TextScrollState *state)
{
    char letter = 0;
    state->charXPosDestination += (CURRENT_CHAR(state).xSize + 5);

    // skip space
    state->currentChar++;
    letter = tolower(state->config->currentText[state->currentChar]);
    // reach end of string
    if (letter == 0)
    {
        return;
    }

    while (letter < 'a' || letter > 'z')
    {
        state->charXPosDestination += 15;
        state->currentChar++;
        letter = tolower(state->config->currentText[state->currentChar]);

        // reach end of string
        if (letter == 0)
        {
            return;
        }
    }

    // found next character, prepare everything for his arrival
    state->charIndex++;
    getCharData(letter, &CURRENT_CHAR(state));
    CURRENT_CHAR(state).xPos = ctx.scrollControlWidth - CURRENT_CHAR(state).xSize;
    CURRENT_CHAR(state).yPos = state->charYPosDestination;

    // save background at character starting position
    saveCharacterBackground(state);
}

//----------------------------------------
static void resetTextController(void)
{
    UBYTE i = 0;
    while (i < TEXT_LIST_SIZE && ctx.scrollStates[i]->config)
    {
        resetTextScrollState(ctx.scrollStates[i]);
        ctx.scrollStates[i]->config = NULL;
        i++;
    }
}

//----------------------------------------
static void resetTextScrollState(struct TextScrollState *state)
{
    UBYTE i = 0;
    for (; i < MAX_CHAR_PER_LINE; i++)
    {
        // Clear character data but keep bitmap allocated
        state->characters[i].xSize = 0;
        state->characters[i].ySize = 0;
        state->characters[i].xPos = 0;
        state->characters[i].yPos = 0;
        state->characters[i].xPosInFont = 0;
        state->characters[i].yPosInFont = 0;
        // oldBackground remains allocated
    }
}

//----------------------------------------
static UWORD displayCurrentCharacter(struct TextScrollState *state)
{
    /*
	 * Don't erase background if character rectangle (B) is blitted into destination (C,D)
	 * Therefore, we use minterm: BC+NBC+BNC -> 1110xxxx -> 0xE0
	 */
    BltBitMap(ctx.fontBlob,
              CURRENT_CHAR(state).xPosInFont, CURRENT_CHAR(state).yPosInFont,
              ctx.textDestination,
              CURRENT_CHAR(state).xPos, CURRENT_CHAR(state).yPos,
              CURRENT_CHAR(state).xSize, CURRENT_CHAR(state).ySize,
              0xC0, 0xff, 0);
    return (UWORD)(state->characters[state->charIndex].xPos + state->characters[state->charIndex].xSize + 5);
}

//----------------------------------------
static void restorePreviousBackground(struct TextScrollState *state)
{
    BltBitMap(CURRENT_CHAR(state).oldBackground,
              0, 0, ctx.textDestination,
              CURRENT_CHAR(state).xPos, CURRENT_CHAR(state).yPos,
              CURRENT_CHAR(state).xSize, CURRENT_CHAR(state).ySize,
              0xC0, 0xff, 0);
}

//----------------------------------------
static void saveCharacterBackground(struct TextScrollState *state)
{
    BltBitMap(ctx.textDestination, CURRENT_CHAR(state).xPos,
              CURRENT_CHAR(state).yPos,
              CURRENT_CHAR(state).oldBackground, 0, 0,
              CURRENT_CHAR(state).xSize,
              CURRENT_CHAR(state).ySize, 0xC0,
              0xff, 0);
}

//----------------------------------------
static void getCharData(char letter, struct CharBlob *charBlob)
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
