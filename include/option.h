#ifndef _OPTION_H_
#define _OPTION_H_

#include "highway.h"
#include "common.h"

typedef struct _hw_option {
    char *root_paths[MAX_PATHS_COUNT];
    int patsh_count;
    char *pattern;
    int pattern_len;
    int worker;
    bool file_with_matches;
} hw_option;

void init_option(int argc, char **argv, hw_option *op);

#endif // _OPTION_H_
