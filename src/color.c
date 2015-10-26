#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "option.h"
#include "color.h"

void printf_with_color(const char *fmt, const char *c, ...)
{
    if (op.color) {
        fputs(c, stdout);
    }

    va_list args;
    va_start(args, c);
    vprintf(fmt, args);
    va_end(args);

    if (op.color) {
        fputs(RESET_COLOR, stdout);
    }
}

void puts_with_color(const char *s, const char *c)
{
    if (op.color) {
        fputs(c, stdout);
    }

    fputs(s, stdout);

    if (op.color) {
        fputs(RESET_COLOR, stdout);
    }
}

void strncat_with_color(char *s1, const char *s2, size_t n, const char *c)
{
    if (op.color) {
        strcat(s1, c);
    }

    strncat(s1, s2, n);

    if (op.color) {
        strcat(s1, RESET_COLOR);
    }
}
