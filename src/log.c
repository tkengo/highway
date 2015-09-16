#include <stdio.h>
#include <stdarg.h>
#include "log.h"

static enum log_level level = LOG_LEVEL_ERROR;

void log_d(const char *fmt, ...)
{
    if (level > LOG_LEVEL_DEBUG) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    printf(fmt, args);
    va_end(args);
}
