#include "demo1.h"
#include "starlight/starlight.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>

WORD payloadShowLogoState = SHOWLOGO_INIT;
ULONG colortable0[COLORMAP32_LONG_SIZE(SHOWLOGO_BLOB_COLORS)];
struct BitMap *showLogoScreen = NULL;
struct BitMap *logo = NULL;
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
        payloadShowLogoState = SHOWLOGO_STATIC;
        break;
    case SHOWLOGO_STATIC:
        //fadeInFromWhite();
        //if (hasFadeInFromWhiteFinished())
        //{
        //    break;
        //}
        break;
    case SHOWLOGO_SHUTDOWN:
        return MODULE_FINISHED;
    }

    return MODULE_CONTINUE;
}

void initShowLogo(void)
{
    UBYTE i = 0;
    writeLog("\n== initShowLogo() ==\n");

    // create the screen
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

    // load colors of screen
    loadColorMap32("img/dawn_320_200_8.CMAP", colortable0, SHOWLOGO_BLOB_COLORS);

    // load logo from file we want to display
    logo = loadBlob("img/dawn_320_200_8.RAW", SHOWLOGO_BLOB_DEPTH,
                    SHOWLOGO_BLOB_WIDTH, SHOWLOGO_LOGO_HEIGHT);
    if (!logo)
    {
        writeLog("Error: Could not allocate memory for logo bitmap\n");
        exitStarlight();
        exitShowLogo();
        exit(RETURN_ERROR);
    }

    // blit logo into screen
    BltBitMap(logo,
              0, 0,
              showLogoScreen,
              0, 0,
              SHOWLOGO_BLOB_WIDTH, SHOWLOGO_LOGO_HEIGHT,
              0xC0, 0xff, 0);

    // everything loaded, now show it!
    writeLog("\nCreate view\n");
    createNewView();
    addViewPort(showLogoScreen, NULL, &colortable0, 
                SHOWLOGO_BLOB_COLORS, TRUE,
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
    if (logo)
    {
        cleanBitMap(logo);
        logo = NULL;
    }
}

void fadeInFromWhite(void)
{
    UWORD i = 0;
    UWORD decrementer;

    // fade of text scroll area (viewPort[0])
    for (; i < SHOWLOGO_BLOB_COLORS; i++)
    {
        decrementer = 0;
        if ((colortable0[i] & 0x000f) != 0x0000)
        {
            decrementer |= 0x0001;
        }
        if ((colortable0[i] & 0x00f0) != 0x0000)
        {
            decrementer |= 0x0010;
        }
        if ((colortable0[i] & 0x0f00) != 0x0000)
        {
            decrementer |= 0x0100;
        }
        colortable0[i] -= decrementer;
    }

    WaitTOF();
    //LoadRGB4(vd.viewPorts[0], colortable0, SHOWLOGO_BLOB_COLORS);
}

BOOL hasFadeInFromWhiteFinished(void)
{
    return TRUE;
}
