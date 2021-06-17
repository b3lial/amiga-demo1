#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <exec/types.h>
#include <graphics/copper.h>
#include <graphics/displayinfo.h>
#include <graphics/gfx.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/gfxnodes.h>
#include <graphics/videocontrol.h>
#include <graphics/view.h>
#include <libraries/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/tagitem.h>

#include "starlight/starlight.h"

__far extern struct Custom custom;

UWORD olddmareq;
UWORD oldintena;
UWORD oldintreq;
UWORD oldadkcon;
ULONG oldview;
ULONG oldcopper;

struct DosBase* DOSBase;
struct GfxBase* GFXBase;
/**
 * Disable Sprites, store old View, init logger.
 * Has to be called before using starlight framework. 
 */
void initStarlight(void){
    DOSBase = (struct DosBase*) OpenLibrary(DOSNAME, 0);
    if(DOSBase==0){
        exit(RETURN_ERROR);
    }
   
    GFXBase = (struct GfxBase*) OpenLibrary(GRAPHICSNAME, 33L);
    if(GFXBase==0){
        printf("could not load %s\n", GRAPHICSNAME);
        exit(RETURN_ERROR);
    }
    
    oldview = (ULONG) GFXBase->ActiView;
    WaitTOF();
    //OFF_SPRITE;
    initLog();
}

/**
 * Restores old View and frees memory. Must be called
 * before exiting. 
 */
void exitStarlight(void){
    WaitTOF();
    //ON_SPRITE;

    // sprite dma working, now we can restore the workbench
    LoadView((struct View*) oldview); 
    WaitTOF();
    
    writeLogFS("Soft Starlight Shutdown successfully\n");

    // we switched back to the old view, so we can now delete
    // the previously created ones
    deleteAllViews();

    // final cleanup and we're gone
    CloseLibrary((struct Library*) GFXBase);
    CloseLibrary((struct Library*) DOSBase);
}
