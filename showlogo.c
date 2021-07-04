#include "demo1.h"
#include "starlight/starlight.h"

#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>

#include <graphics/gfxmacros.h>
#include <hardware/custom.h>

WORD payloadShowLogoState = SHOWLOGO_INIT;
struct BitMap *logo = NULL;
struct Screen *logoscreen0 = NULL;
UWORD dawnPaletteRGB4[256] =
    {
        0x0011, 0x0489, 0x0147, 0x0024, 0x0344, 0x0456, 0x0556, 0x0666,
        0x0666, 0x0666, 0x0666, 0x0555, 0x0445, 0x0444, 0x0444, 0x08AB,
        0x06CD, 0x028D, 0x058C, 0x05AE, 0x0222, 0x06AE, 0x0122, 0x07CC,
        0x07DE, 0x0111, 0x05AE, 0x0CDD, 0x07CE, 0x07DE, 0x0111, 0x09DD,
        0x0ADD, 0x05AD, 0x0111, 0x0457, 0x069C, 0x059C, 0x0111, 0x0111,
        0x05BF, 0x09EE, 0x07CE, 0x016A, 0x07BF, 0x07CF, 0x09EE, 0x06AE,
        0x06AD, 0x08DE, 0x059D, 0x03AF, 0x06BF, 0x06AF, 0x069D, 0x0146,
        0x08CF, 0x08DF, 0x029E, 0x06AE, 0x07BF, 0x07BF, 0x0AFF, 0x0234,
        0x0367, 0x0BEE, 0x06AE, 0x048C, 0x0479, 0x07CF, 0x08EF, 0x08FF,
        0x06AF, 0x0036, 0x07CF, 0x09FF, 0x059D, 0x0AFF, 0x08DF, 0x0011,
        0x059D, 0x017C, 0x048D, 0x0222, 0x0689, 0x0011, 0x07BF, 0x0333,
        0x0222, 0x08FF, 0x0257, 0x06AD, 0x07CF, 0x0122, 0x06AE, 0x048B,
        0x06BF, 0x0011, 0x0AFF, 0x037A, 0x0134, 0x08EE, 0x0EFF, 0x0567,
        0x069D, 0x05AE, 0x059A, 0x05AD, 0x0011, 0x06BF, 0x058A, 0x08CF,
        0x08DF, 0x0BFF, 0x09FF, 0x0BFF, 0x07CF, 0x059C, 0x08DF, 0x08DF,
        0x08EF, 0x0159, 0x06BF, 0x09FF, 0x07CF, 0x059D, 0x048C, 0x09EF,
        0x069E, 0x09CD, 0x06AB, 0x0AAA, 0x0269, 0x0478, 0x06DF, 0x0245,
        0x068B, 0x0578, 0x07AC, 0x057A, 0x0ACC, 0x0CFF, 0x0689, 0x049E,
        0x0369, 0x08BC, 0x07AB, 0x08CC, 0x0256, 0x08EF, 0x05CF, 0x038C,
        0x0789, 0x047B, 0x0257, 0x06BC, 0x06AD, 0x0358, 0x04AF, 0x0479,
        0x058A, 0x0378, 0x0468, 0x059B, 0x08BB, 0x039F, 0x058B, 0x027A,
        0x0456, 0x0678, 0x0245, 0x048B, 0x08CD, 0x0BDD, 0x07BB, 0x028C,
        0x0135, 0x069A, 0x057A, 0x037B, 0x07CD, 0x0DEE, 0x038C, 0x07BE,
        0x06AD, 0x04BF, 0x079B, 0x09DD, 0x0356, 0x0357, 0x068A, 0x0678,
        0x059A, 0x0568, 0x06AD, 0x07BC, 0x0579, 0x0369, 0x06CF, 0x06AA,
        0x0467, 0x049D, 0x049E, 0x07DF, 0x0258, 0x049B, 0x0158, 0x07BD,
        0x0334, 0x0368, 0x0233, 0x016B, 0x0456, 0x0ADE, 0x07AB, 0x0379,
        0x0035, 0x039E, 0x0146, 0x068B, 0x08CF, 0x0578, 0x047A, 0x0123,
        0x068A, 0x027B, 0x0456, 0x07CD, 0x0345, 0x0779, 0x08DD, 0x027C,
        0x06AB, 0x0BCC, 0x0146, 0x06BD, 0x07EF, 0x038D, 0x037A, 0x0135,
        0x0269, 0x059C, 0x09CC, 0x0DFF, 0x08AB, 0x07AD, 0x04AF, 0x0023,
        0x068A, 0x0357, 0x069A, 0x07BE, 0x06AE, 0x089A, 0x0356, 0x0BDE};
UWORD *color0 = NULL;

__far extern struct Custom custom;

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
        fadeInFromWhite();
        break;
    case SHOWLOGO_SHUTDOWN:
        return MODULE_FINISHED;
    }

    return MODULE_CONTINUE;
}

void initShowLogo(void)
{
    UWORD i = 0;
    writeLog("\n== initShowLogo() ==\n");

    // load logo from file we want to display
    logo = loadBlob("img/dawn_320_256_8.RAW", SHOWLOGO_BLOB_DEPTH,
                    SHOWLOGO_BLOB_WIDTH, SHOWLOGO_BLOB_HEIGHT);
    if (!logo)
    {
        writeLog("Error: Could not allocate memory for logo bitmap\n");
        exitShowLogo();
        exit(RETURN_ERROR);
    }

    // this color table will fade from white to logo
    color0 = AllocVec(sizeof(dawnPaletteRGB4), NULL);
    if (!color0)
    {
        writeLog("Error: Could not allocate memory for logo color table\n");
        exitShowLogo();
        exit(RETURN_ERROR);
    }
    for (; i < SHOWLOGO_BLOB_COLORS; i++)
    {
        color0[i] = 0x0fff;
    }

    writeLog("\nCreate screen\n");
    
    // we create two doublebuffer screens for flicker free color dim operations
    logoscreen0 = createScreen(logo, TRUE, 0, 0,
        SHOWLOGO_BLOB_WIDTH, SHOWLOGO_BLOB_HEIGHT,
        SHOWLOGO_BLOB_DEPTH, NULL);
    LoadRGB4(&logoscreen0->ViewPort, color0, SHOWLOGO_BLOB_COLORS);

    // Make Screens visible
    ScreenToFront(logoscreen0);
    OFF_SPRITE;
}

void exitShowLogo(void)
{
    if (logoscreen0){
        CloseScreen(logoscreen0);
        logoscreen0 = NULL;
    }
    WaitTOF();
    ON_SPRITE;

    if (logo)
    {
        cleanBitMap(logo);
        logo = NULL;
    }
    if (color0)
    {
        FreeVec(color0);
        color0 = NULL;
    }
}

void fadeInFromWhite(void)
{
    UWORD decrementer;
    UWORD i = 0;

    // fade effect on color table
    for (; i < SHOWLOGO_BLOB_COLORS; i++)
    {
        decrementer = 0;
        if ((color0[i] & 0x000f) != (dawnPaletteRGB4[i] & 0x000f))
        {
            decrementer |= 0x0001;
        }
        if ((color0[i] & 0x00f0) != (dawnPaletteRGB4[i] & 0x00f0))
        {
            decrementer |= 0x0010;
        }
        if ((color0[i] & 0x0f00) != (dawnPaletteRGB4[i] & 0x0f00))
        {
            decrementer |= 0x0100;
        }
        color0[i] -= decrementer;
    }

    // show result of fade in one of the screen buffers and show it
    WaitTOF();
    while(TRUE){
        if(VBeamPos() > 256){
            break;
        }
    }
    LoadRGB4(&logoscreen0->ViewPort, color0, SHOWLOGO_BLOB_COLORS);
    OFF_SPRITE;
}