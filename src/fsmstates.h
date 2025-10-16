#ifndef __FSMSTATES_H__
#define __FSMSTATES_H__

enum MainFSMState {
    FSM_START = 0,
    FSM_TEXTSCROLLER = 1,
    FSM_TEXTSCROLLER_FINISHED = 2,
    FSM_SHOWLOGO = 3,
    FSM_STOP = 4,
    FSM_ERROR = 5,
    FSM_QUIT = 6
};

#endif
