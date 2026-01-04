#include <clib/exec_protos.h>
#include <devices/timer.h>
#include <exec/io.h>
#include <exec/types.h>

#include "timecontroller.h"

struct MsgPort *timerPort = NULL;
struct timerequest *timerReq = NULL;

BOOL openTimer(void) {
    timerPort = CreateMsgPort();
    if (timerPort == NULL) {
        return FALSE;
    }

    timerReq = (struct timerequest *)CreateIORequest(timerPort, sizeof(struct timerequest));
    if (timerReq == NULL) {
        DeleteMsgPort(timerPort);
        timerPort = NULL;
        return FALSE;
    }

    if (OpenDevice((CONST_STRPTR)TIMERNAME, UNIT_MICROHZ, (struct IORequest *)timerReq, 0) != 0) {
        DeleteIORequest((struct IORequest *)timerReq);
        DeleteMsgPort(timerPort);
        timerReq = NULL;
        timerPort = NULL;
        return FALSE;
    }

    return TRUE;
}

void closeTimer(void) {
    if (timerReq != NULL) {
        CloseDevice((struct IORequest *)timerReq);
        DeleteIORequest((struct IORequest *)timerReq);
        timerReq = NULL;
    }

    if (timerPort != NULL) {
        DeleteMsgPort(timerPort);
        timerPort = NULL;
    }
}
