// Copyright 2021 Christian Ammann

#ifndef __DEMO_1_H__
#define __DEMO_1_H__

#define MAX_CHAR_HEIGHT 33
#define MAX_CHAR_WIDTH 30
#define DEMO_STR_MAX 120

#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/graphics_protos.h>
#include <clib/intuition_protos.h>
#include <ctype.h>
#include <dos/dos.h>
#include <exec/memory.h>
#include <exec/types.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <graphics/gfxmacros.h>
#include <graphics/rastport.h>
#include <graphics/videocontrol.h>
#include <hardware/cia.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunkyconverter/chunkyconverter.h"

#include "main.h"
#include "showlogo.h"
#include "textscroller.h"
#include "textcontroller.h"

#include "rotation/rotation.h"

#include "starlight/blob_controller.h"
#include "starlight/graphics_controller.h"
#include "starlight/utils.h"

#endif
