#include "demo1.h"
#include "starlight/starlight.h"

#include <exec/types.h>

WORD payloadShowLogoState = SHOWLOGO_INIT;

WORD fsmShowLogo(void){
    return MODULE_FINISHED;
    
    if (mouseClick())
    {
        payloadShowLogoState = SHOWLOGO_SHUTDOWN;
    }

    switch(payloadShowLogoState){
        case SHOWLOGO_INIT:
            payloadShowLogoState = SHOWLOGO_STATIC;
            break;
        case SHOWLOGO_STATIC:
            //WaitTOF();
            break;
        case SHOWLOGO_SHUTDOWN:
            return MODULE_FINISHED;
    }

    return MODULE_CONTINUE;
}