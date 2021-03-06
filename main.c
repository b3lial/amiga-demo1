// Copyright 2021 Christian Ammann

#include "demo1.h"

WORD fsmCurrentState = FSM_START;
WORD fsmNextState = -1;

// empty mouse pointer because we dont want to see a mouse
UWORD *emptyPointer;

int main(void) {
    struct Screen *my_wbscreen_ptr;

    // requires aga for 8 bitplanes 24 bit colors
    if (!isAga()) {
        printf("Error, this demo requires an aga chipset to run\n");
        exit(RETURN_ERROR);
    }

    // hide mouse
    emptyPointer = AllocVec(22 * sizeof(UWORD), MEMF_CHIP | MEMF_CLEAR);
    my_wbscreen_ptr = LockPubScreen("Workbench");
    SetPointer(my_wbscreen_ptr->FirstWindow, emptyPointer, 8, 8, -6, 0);
    UnlockPubScreen(NULL, my_wbscreen_ptr);

    // write logfile to ram: if debug is enabled
    initLog();

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

            case FSM_STOP:
                exitShowLogo();
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

    // restore mouse on every window
    my_wbscreen_ptr = LockPubScreen("Workbench");
    ClearPointer(my_wbscreen_ptr->FirstWindow);
    UnlockPubScreen(NULL, my_wbscreen_ptr);
    FreeVec(emptyPointer);

    exit(RETURN_OK);
}

int isAga(void) {
    short int *vposr = (short int *)0xdff004;

    return *vposr & (1 << 9);
}
