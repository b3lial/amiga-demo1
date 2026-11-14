#include <exec/types.h>
#include <exec/memory.h>
#include <graphics/gfx.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <clib/exec_protos.h>

#include "plasma.h"
#include "fsmstates.h"
#include "utils/utils.h"

struct PlasmaContext {
    enum PlasmaState state;
};

static struct PlasmaContext ctx = {
    .state = PLASMA_INIT
};

UWORD fsmPlasma(void) {
    if (mouseClick()) {
        ctx.state = PLASMA_EXIT;
    }

    switch (ctx.state) {
        case PLASMA_INIT:
            ctx.state = PLASMA_ANIMATE;
            break;
        case PLASMA_ANIMATE:
            // TODO: implement plasma effect
            break;
        case PLASMA_EXIT:
            return FSM_PLASMA_FINISHED;
    }

    return FSM_PLASMA;
}

UWORD initPlasma(void) {
    writeLog("\n\n== initPlasma() ==\n");

    // TODO: allocate bitmaps, create screens, setup buffers

    return FSM_PLASMA;
}

void exitPlasma(void) {
    writeLog("\n== exitPlasma() ==\n");

    // TODO: free allocated resources

    ctx.state = PLASMA_INIT;
}
