#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "log.h"
#include "color.h"

static enum log_level level = LOG_LEVEL_ERROR;
static FILE *log_buffer_fd = NULL;

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
    printf("\n");
    va_end(args);
}

void log_buffered(const char *fmt, ...)
{
    if (log_buffer_fd == NULL) {
        log_buffer_fd = tmpfile();
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(log_buffer_fd, fmt, args);
    fprintf(log_buffer_fd, "\n");
    va_end(args);
}

void log_flush()
{
    if (log_buffer_fd == NULL) {
        return;
    }

    char buf[1024];
    rewind(log_buffer_fd);
    while (fgets(buf, 1024, log_buffer_fd) != NULL) {
        printf("%s", buf);
    }
    fclose(log_buffer_fd);
    log_buffer_fd = NULL;
}
