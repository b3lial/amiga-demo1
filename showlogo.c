#include "demo1.h"
#include "starlight/starlight.h"

#include <exec/types.h>

WORD fsmShowLogo(void){
    WaitTOF();
    if (mouseClick())
    {
        return MODULE_FINISHED;
    }

    return MODULE_CONTINUE;
}