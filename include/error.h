#ifndef UCD_ERROR_H
#define UCD_ERROR_H

#include <stdarg.h>

#define UCD_ERROR_BUFFER_SIZE 128

enum {
    UCD_ERROR_FMT_OFF,
    UCD_ERROR_FMT_ANSI,
};

enum {
    UCD_ERROR_WARN,
    UCD_ERROR_ABORT,
    UCD_ERROR_FATAL,
};

void error_msg(int level, int errnum, const char *msg, ...);
void error_msg_v(int level, int errnum, const char *msg, va_list args);
void error_sys(int level, int errnum, const char *msg, ...);
void error_sys_v(int level, int errnum, const char *msg, va_list args);

#endif
