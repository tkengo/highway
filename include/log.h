#ifndef HW_LOG_H
#define HW_LOG_H

enum log_level {
    LOG_LEVEL_DEBUG = 1,
    LOG_LEVEL_ERROR
};

void set_log_level(enum log_level l);
void log_e(const char *fmt, ...);
void log_w(const char *fmt, ...);
void log_d(const char *fmt, ...);
void log_flush();

#endif // HW_LOG_H
