#ifndef _LOG_H_
#define _LOG_H_

enum log_level {
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_ERROR
};

void log_d(const char *fmt, ...);

#endif // _LOG_H_
