#include <stdio.h>
#include <stdarg.h>
#include "log.h"
#include "color.h"

static enum log_level level = LOG_LEVEL_ERROR;

void set_log_level(enum log_level l)
{
    level = l;
}

void log_e(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "%sFatal%s: ", ERROR_COLOR, RESET_COLOR);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void log_d(const char *fmt, ...)
{
    if (level > LOG_LEVEL_DEBUG) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}
