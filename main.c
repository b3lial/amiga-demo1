#include <exec/types.h>
#include <dos/dos.h>

#include "starlight/starlight.h"
#include "main.h"
#include "textscroller.h"

WORD fsmCurrentState = FSM_START;
WORD fsmNextState = -1;

int main(void)
{
    while(fsmCurrentState!=FSM_QUIT){
        UWORD moduleStatus = NULL;
        
        switch(fsmCurrentState){
            case FSM_START:
                initSystem(TRUE);
                fsmNextState = FSM_TEXTSCROLLER;
                break;
            
            case FSM_TEXTSCROLLER:
                moduleStatus = fsmTextScroller();
                if(moduleStatus==MODULE_CONTINUE){
                    fsmNextState = FSM_TEXTSCROLLER;
                }
                else{
                    fsmNextState = FSM_STOP;
                }
                break;

            case FSM_STOP:
                fsmNextState = FSM_QUIT;
                break;
            
            //something unexpected happened, we better leave
            default:
                fsmNextState = FSM_QUIT;
                writeLogFS("Error: Main, unknown fsm status %d\n",
                        fsmCurrentState);
                break;
        }
    
        fsmCurrentState = fsmNextState;        
    }

    exitSystem(RETURN_OK);
}
