#ifndef __MANDELBROT_H__
#define __MANDELBROT_H__

#include <exec/types.h>

enum MandelbrotState {
    MANDELBROT_INIT = 0,
    MANDELBROT_SHOW = 1,
    MANDELBROT_EXIT = 2
};

#define MANDELBROT_SCREEN_WIDTH  320
#define MANDELBROT_SCREEN_HEIGHT 256
#define MANDELBROT_SCREEN_DEPTH  8
#define MANDELBROT_SCREEN_COLORS 256
#define MANDELBROT_MAX_ITER      500


UWORD fsmMandelbrot(void);
UWORD initMandelbrot(void);
void exitMandelbrot(void);

#endif
