#include "error.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static _Thread_local unsigned g_error_fmt = UCD_ERROR_FMT_ANSI;

static inline const char *strerror_x(int errnum)
{
    static _Thread_local char error_buffer[UCD_ERROR_BUFFER_SIZE];
#ifdef _GNU_SOURCE
    return strerror_r(errnum, error_buffer, UCD_ERROR_BUFFER_SIZE);
#else
    int status = strerror_r(errnum, error_buffer, UCD_ERROR_BUFFER_SIZE);
    switch (status) {
    case EINVAL:
        return strerror_x(EINVAL);
    case ERANGE:
        if (errnum == ERANGE) {
            error_msg(
                UCD_ERROR_FATAL, ERANGE,
                "Value of UCD_ERROR_BUFFER_SIZE is too small");
        }
        return strerror_x(ERANGE);
    default:
        return error_buffer;
    }
#endif
}

static inline const char *get_level_fmt_ansi(int level)
{
    switch (level) {
    case UCD_ERROR_WARN:
        return "\x1b[96;1m";
    case UCD_ERROR_ABORT:
        return "\x1b[95;1m";
    case UCD_ERROR_FATAL:
        return "\x1b[91;1m";
    default:
        assert(
            level == UCD_ERROR_WARN || level == UCD_ERROR_ABORT ||
            level == UCD_ERROR_FATAL);
        return "";
    }
}

static inline const char *get_level_fmt(int level)
{
    switch (g_error_fmt) {
    case UCD_ERROR_FMT_ANSI:
        return get_level_fmt_ansi(level);
    default:
        return "";
    }
}

static inline const char *get_reset_fmt(void)
{
    switch (g_error_fmt) {
    case UCD_ERROR_FMT_ANSI:
        return "\x1b[0m";
    default:
        return "";
    }
}

static inline const char *get_level_string(int level)
{
    switch (level) {
    case UCD_ERROR_WARN:
        return "warning";
    case UCD_ERROR_ABORT:
        return "abort";
    case UCD_ERROR_FATAL:
        return "fatal";
    default:
        assert(
            level == UCD_ERROR_WARN || level == UCD_ERROR_ABORT ||
            level == UCD_ERROR_FATAL);
        return "";
    }
}

static inline void begin_msg_v(int level, const char *msg, va_list args)
{
    fprintf(
        stderr, "ucd: %s%s%s: ", get_level_fmt(level), get_level_string(level),
        get_reset_fmt());
    vfprintf(stderr, msg, args);
}

static inline void end_msg(int level)
{
    fprintf(stderr, "\n");
    if (level == UCD_ERROR_FATAL) {
        exit(EXIT_FAILURE);
    }
}

void error_msg(int level, int errnum, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    error_msg_v(level, errnum, msg, args);
    va_end(args);
}

void error_msg_v(int level, int errnum, const char *msg, va_list args)
{
    begin_msg_v(level, msg, args);
    end_msg(level);
}

void error_sys(int level, int errnum, const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    error_sys_v(level, errnum, msg, args);
    va_end(args);
}

void error_sys_v(int level, int errnum, const char *msg, va_list args)
{
    begin_msg_v(level, msg, args);
    fprintf(stderr, ": %s", strerror_x(errnum));
    end_msg(level);
}
