#ifndef __TIMECONTROLLER_H__
#define __TIMECONTROLLER_H__

#include <exec/types.h>

/**
 * @brief Initialize and open the timer device
 */
BOOL openTimer(void);

/**
 * @brief Close the timer device and free resources
 */
void closeTimer(void);

#endif
