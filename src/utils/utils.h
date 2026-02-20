#ifndef __UTILS_H__
#define __UTILS_H__

#include <exec/types.h>

#define DEMO_STR_MAX 120

BOOL mouseClick(void);
BOOL mouseCiaStatus(void);

/**
 * @brief Write format string into logfile
 */
BOOL writeLogFS(const char* formatString, ...);

/**
 * @brief Writes msg and a character array into log file. Maybe not the most performant implementation but who cares.
 */
BOOL writeArrayLog(char*, unsigned char*, UWORD);

BOOL initLog(void);

/**
 * @brief Write string into logfile
 */
BOOL writeLog(char*);

/**
 * @brief Convert fixed-point WORD to string representation
 * @param fixedVal The fixed-point value (8.8 format)
 * @param buffer Output buffer (must be at least 12 bytes)
 * @return Pointer to the buffer
 */
char* fixToStr(WORD fixedVal, char* buffer);

#endif
