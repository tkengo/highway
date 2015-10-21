#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <pthread.h>
#include "highway.h"
#include "log.h"
#include "util.h"
#include "color.h"

#ifdef _WIN32
//#define fprintf(...) fprintf_w32(__VA_ARGS__)
#endif

static enum log_level level = LOG_LEVEL_ERROR;
static FILE *log_buffer_fd = NULL;
static pthread_mutex_t log_mutex;

void set_log_level(enum log_level l)
{
    level = l;
}

FILE *init_buffer_fd()
{
    if (log_buffer_fd == NULL) {
        log_buffer_fd = tmpfile();

        if (log_buffer_fd == NULL && errno == EMFILE) {
            if (!set_fd_rlimit(MAX_FD_NUM + 100)) {
                log_buffer_fd = stderr;
            } else {
                log_buffer_fd = tmpfile();
            }
        }

        if (log_buffer_fd != NULL) {
            pthread_mutex_init(&log_mutex, NULL);
        }
    }

    return log_buffer_fd;
}

void log_e(const char *fmt, ...)
{
    FILE *fd = init_buffer_fd();

    pthread_mutex_lock(&log_mutex);
    va_list args;
    va_start(args, fmt);
    fprintf(fd, "%sFatal%s: ", ERROR_COLOR, RESET_COLOR);
    vfprintf(fd, fmt, args);
    fputs("\n", fd);
    va_end(args);
    pthread_mutex_unlock(&log_mutex);
}

void log_w(const char *fmt, ...)
{
    FILE *fd = init_buffer_fd();

    pthread_mutex_lock(&log_mutex);
    va_list args;
    va_start(args, fmt);
    fprintf(fd, "%sWarning%s: ", WARNING_COLOR, RESET_COLOR);
    vfprintf(fd, fmt, args);
    fputs("\n", fd);
    va_end(args);
    pthread_mutex_unlock(&log_mutex);
}

void log_d(const char *fmt, ...)
{
    if (level > LOG_LEVEL_DEBUG) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    puts("\n");
    va_end(args);
}

void log_flush()
{
    if (log_buffer_fd == NULL) {
        return;
    }

    const int BUF_SIZE = 2048;
    char buf[BUF_SIZE];
    rewind(log_buffer_fd);
    while (fgets(buf, BUF_SIZE, log_buffer_fd) != NULL) {
        printf("%s", buf);
    }
    fclose(log_buffer_fd);
    log_buffer_fd = NULL;
}
