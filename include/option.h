#ifndef _HW_OPTION_H_
#define _HW_OPTION_H_

#include "highway.h"
#include "common.h"

typedef struct _hw_option {
    char *root_paths[MAX_PATHS_COUNT]; /* path list for searching */
    int paths_count;                   /* path count above list */
    char *pattern;
    int pattern_len;
    int worker;
    bool file_with_matches;
    bool use_regex;
    bool all_files;
    bool no_omit;
    bool ignore_case;
} hw_option;

void init_option(int argc, char **argv, hw_option *op);
void free_option(hw_option *op);

#endif // _HW_OPTION_H_
