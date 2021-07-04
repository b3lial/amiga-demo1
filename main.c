// Copyright 2021 Christian Ammann

#include "demo1.h"

WORD fsmCurrentState = FSM_START;
WORD fsmNextState = -1;

int main(void) {
    initLog();

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

    exit(RETURN_OK);
}
