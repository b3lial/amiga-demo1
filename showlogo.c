#include "demo1.h"
#include "starlight/starlight.h"

#include <exec/types.h>
#include <dos/dos.h>

WORD payloadShowLogoState = SHOWLOGO_INIT;
UWORD colortable0[SHOWLOGO_BLOB_COLORS];
struct BitMap *showLogoScreen;
extern struct ViewData vd;

WORD fsmShowLogo(void)
{
    if (mouseClick())
    {
        payloadShowLogoState = SHOWLOGO_SHUTDOWN;
    }

    switch (payloadShowLogoState)
    {
    case SHOWLOGO_INIT:
        initShowLogo();
        payloadShowLogoState = SHOWLOGO_STATIC;
        break;
    case SHOWLOGO_STATIC:
        /*
        fadeInFromWhite();
        if(hasFadeInFromWhiteFinished()){
            break;
        }
        */
        break;
    case SHOWLOGO_SHUTDOWN:
        exitShowLogo();
        return MODULE_FINISHED;
    }

    return MODULE_CONTINUE;
}

void initShowLogo(void)
{
    UBYTE i = 0;
    writeLog("\n== initShowLogo() ==\n");
    memset(colortable0, 0xff, sizeof(colortable0));

    writeLog("\nLoad showlogo screen background bitmap\n");
    showLogoScreen = createBitMap(SHOWLOGO_BLOB_DEPTH,
                                  SHOWLOGO_BLOB_WIDTH,
                                  SHOWLOGO_BLOB_HEIGHT);
    if (!showLogoScreen)
    {
        writeLog("Error: Could not allocate memory for showlogo screen bitmap\n");
        exitStarlight();
        exitShowLogo();
        exit(RETURN_ERROR);
    }
    for (i = 0; i < SHOWLOGO_BLOB_DEPTH; i++)
    {
        BltClear(showLogoScreen->Planes[i],
                 (showLogoScreen->BytesPerRow) * (showLogoScreen->Rows),
                 1);
    }

    writeLog("\nCreate view\n");
    createNewView();
    addViewPort(showLogoScreen, NULL, colortable0, SHOWLOGO_BLOB_COLORS, FALSE,
                0, 0, SHOWLOGO_BLOB_WIDTH, SHOWLOGO_BLOB_HEIGHT,
                0, 0);
    startView();
}

void exitShowLogo(void)
{
    if (showLogoScreen)
    {
        cleanBitMap(showLogoScreen);
        showLogoScreen = NULL;
    }
}

void fadeInFromWhite(void){
    UWORD i = 0;
    UWORD decrementer;

    // fade of text scroll area (viewPort[0])
    for(;i<SHOWLOGO_BLOB_COLORS;i++){
        decrementer = 0;
        if((colortable0[i] & 0x000f) != 0x0000){
            decrementer |= 0x0001;
        }
        if((colortable0[i] & 0x00f0) != 0x0000){
            decrementer |= 0x0010;
        }
        if((colortable0[i] & 0x0f00) != 0x0000){
            decrementer |= 0x0100;
        }
        colortable0[i]-=decrementer;
    }

    WaitTOF();
    LoadRGB4(vd.viewPorts[0], colortable0, SHOWLOGO_BLOB_COLORS);
}

BOOL hasFadeInFromWhiteFinished(void){
    return TRUE;
}