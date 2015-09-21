#include <string.h>
#include <ctype.h>

char *trim(char *str)
{
    while (isspace(*str)) str++;
    if (*str == '\0') {
        return str;
    }

    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;
    *(end + 1) = '\0';

    return str;
}
