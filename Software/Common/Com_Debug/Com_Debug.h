#ifndef COM_DEBUG_H
#define COM_DEBUG_H

#include "uart.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define DEBUG_ENABLE

#ifdef DEBUG_ENABLE
#define FILE_NAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define __FILE_NAME__ (strrchr(FILE_NAME, '/') ? strrchr(FILE_NAME, '/') + 1 : FILE_NAME)
#define Debug_sendln(format, ...) printf("[%s:%d]" format "\r\n", __FILE_NAME__, __LINE__, ##__VA_ARGS__)
#else
#define FILE_NAME
#define __FILE_NAME__
#define Debug_sendln(format, ...)

#endif

void Com_Debug_Init(void);

#endif // COM_DEBUG_H
