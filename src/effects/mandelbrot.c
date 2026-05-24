#include <exec/types.h>

#include "mandelbrot.h"
#include "fsmstates.h"

//----------------------------------------
UWORD initMandelbrot(void) {
    return FSM_MANDELBROT;
}

//----------------------------------------
UWORD fsmMandelbrot(void) {
    return FSM_MANDELBROT;
}

//----------------------------------------
void exitMandelbrot(void) {
}
