// Copyright 2021 Christian Ammann

#include "demo1.h"

WORD fsmCurrentState = FSM_START;
WORD fsmNextState = -1;

// empty mouse pointer because we dont want to see a mouse
UWORD *emptyPointer;

int main(void) {
    struct Screen  *my_wbscreen_ptr;

    // requires aga for 8 bitplanes 24 bit colors
    if(!isAga()){
        printf("Error, this demo requires an aga chipset to run\n");
        exit(RETURN_ERROR);
    }

    // hide mouse
    emptyPointer = AllocVec(22*sizeof(UWORD), MEMF_CHIP | MEMF_CLEAR);
    my_wbscreen_ptr = LockPubScreen("Workbench");
    SetPointer(my_wbscreen_ptr->FirstWindow, emptyPointer, 8, 8, -6, 0);
    UnlockPubScreen(NULL, my_wbscreen_ptr);

    // write logfile to ram: if debug is enabled
    initLog();

    // main loop which inits screens and executes effects
    while (fsmCurrentState != FSM_QUIT) {
        UWORD moduleStatus = NULL;

        switch (fsmCurrentState) {
            case FSM_START:
                initTextScroller();
                fsmNextState = FSM_TEXTSCROLLER;
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
                exitTextScroller();
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

    // restore mouse on every window
    my_wbscreen_ptr = LockPubScreen("Workbench");
    ClearPointer(my_wbscreen_ptr->FirstWindow);
    UnlockPubScreen(NULL, my_wbscreen_ptr);
    FreeVec(emptyPointer);

    exit(RETURN_OK);
}

int isAga(void) {
  short int *vposr = (short int*) 0xdff004;

  return *vposr & (1<<9);
}
