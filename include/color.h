#ifndef HW_COLOR_H
#define HW_COLOR_H

#define ERROR_COLOR "\033[31m"
#define WARNING_COLOR "\033[33m"
#define OMIT_COLOR "\033[31m"
#define RESET_COLOR "\033[39;49;0m"

#define OMIT_COLOR_LEN 5
#define RESET_COLOR_LEN 14
#define OMIT_ESCAPE_LEN (OMIT_COLOR_LEN + RESET_COLOR_LEN)

#define PATH_COLOR "32;1"
#define MATCH_COLOR "31"
#define LINE_NUMBER_COLOR "1"
#define BEFORE_CONTEXT_COLOR "33"
#define AFTER_CONTEXT_COLOR "35"
#define COLOR_ESCAPE_SEQUENCE "\e[%sm"

void printf_with_color(const char *fmt, const char *c, ...);
void puts_with_color(const char *s, const char *c);
void strncat_with_color(char *s1, const char *s2, size_t n, const char *c);

#endif // HW_COLOR_H
