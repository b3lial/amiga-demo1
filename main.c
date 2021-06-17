// Copyright 2021 Christian Ammann

#include "demo1.h"

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

WORD fsmCurrentState = FSM_START;
WORD fsmNextState = -1;

int main(void) {
    while (fsmCurrentState != FSM_QUIT) {
        UWORD moduleStatus = NULL;

        switch (fsmCurrentState) {
            case FSM_START:
                initStarlight();
                //initTextScroller();
                fsmNextState = FSM_TEXTSCROLLER_FINISHED;
                break;

            case FSM_TEXTSCROLLER:
                moduleStatus = fsmTextScroller();
                if (moduleStatus == MODULE_CONTINUE) {
                    fsmNextState = FSM_TEXTSCROLLER;
                } else {
                    fsmNextState = FSM_TEXTSCROLLER_FINISHED;
                }
                break;

            case FSM_TEXTSCROLLER_FINISHED:
                initShowLogo();
                //exitTextScroller();
                fsmNextState = FSM_SHOWLOGO;
                break;

            case FSM_SHOWLOGO:
                moduleStatus = fsmShowLogo();
                if (moduleStatus == MODULE_CONTINUE) {
                    fsmNextState = FSM_SHOWLOGO;
                } else {
                    fsmNextState = FSM_STOP;
                }
                break;

            case FSM_STOP:
                exitStarlight();
                exitShowLogo();
                fsmNextState = FSM_QUIT;
                break;

            // something unexpected happened, we better leave
            default:
                fsmNextState = FSM_QUIT;
                writeLogFS("Error: Main, unknown fsm status %d\n",
                           fsmCurrentState);
                break;
        }

        fsmCurrentState = fsmNextState;
    }

    exit(RETURN_OK);
}
