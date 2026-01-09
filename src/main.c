#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <stdlib.h>
#include <dos/dos.h>

#include "main.h"

#include "fsmstates.h"
#include "utils/utils.h"
#include "utils/timecontroller.h"
#include "effects/textscroller.h"
#include "effects/showlogo.h"
#include "effects/rotatingcube.h"

int main(void) {
    enum MainFSMState fsmCurrentState = FSM_START;
    enum MainFSMState fsmNextState = FSM_ERROR;
    UWORD *emptyPointer = NULL;
    struct Screen *my_wbscreen_ptr;
    int returnValue = RETURN_OK;

    // requires aga for 8 bitplanes 24 bit colors
    if (!isAga()) {
        returnValue = RETURN_ERROR;
        goto __exit_demo;
    }

    // hide mouse
    emptyPointer = AllocVec(22 * sizeof(UWORD), MEMF_CHIP | MEMF_CLEAR);
    if(emptyPointer == NULL) {
        returnValue = RETURN_ERROR;
        goto __exit_demo;
    }
    my_wbscreen_ptr = LockPubScreen((CONST_STRPTR)"Workbench");
    SetPointer(my_wbscreen_ptr->FirstWindow, emptyPointer, 8, 8, -6, 0);
    UnlockPubScreen(NULL, my_wbscreen_ptr);

    // write logfile to ram: if debug is enabled
    initLog();

    // open timer device
    if (!openTimer()) {
        writeLogFS("Error: Failed to open timer\n");
        returnValue = RETURN_ERROR;
        goto __cleanup_demo;
    }

    // test time to make sure its really working
    if(getSystemTime() == 0)
    {
        writeLogFS("Current time is 0. RTC seems not to work\n");
        returnValue = RETURN_ERROR;
        goto __cleanup_demo;
    }

    // main loop which inits screens and executes effects
    while (fsmCurrentState != FSM_QUIT) {
        switch (fsmCurrentState) {
            case FSM_START:
                fsmNextState = initTextScroller();
                break;

            case FSM_TEXTSCROLLER:
                fsmNextState = fsmTextScroller();
                break;

            case FSM_TEXTSCROLLER_FINISHED:
                fsmNextState = initShowLogo();
                exitTextScroller();
                break;

            case FSM_SHOWLOGO:
                fsmNextState = fsmShowLogo();
                break;

            case FSM_SHOWLOGO_FINISHED:
                fsmNextState = initRotatingCube();
                exitShowLogo();
                break;

            case FSM_ROTATINGCUBE:
                fsmNextState = fsmRotatingCube();
                break;

            case FSM_STOP:
                exitRotatingCube();
                fsmNextState = FSM_QUIT;
                break;

            case FSM_ERROR:
                fsmNextState = FSM_QUIT;
                writeLogFS("Error: Main, submodule in error state\n");
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

__cleanup_demo:
    // close timer device
    closeTimer();

    // restore mouse on every window
    my_wbscreen_ptr = LockPubScreen((CONST_STRPTR)"Workbench");
    ClearPointer(my_wbscreen_ptr->FirstWindow);
    UnlockPubScreen(NULL, my_wbscreen_ptr);
    FreeVec(emptyPointer);

__exit_demo:
    exit(returnValue);
}

//----------------------------------------
int isAga(void) {
    short int *vposr = (short int *)0xdff004;

    return *vposr & (1 << 9);
}
