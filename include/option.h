#ifndef HW_OPTION_H
#define HW_OPTION_H

#include "highway.h"
#include "common.h"

#define MIN_LINE_LENGTH 100

typedef struct _hw_option {
    char *root_paths[MAX_PATHS_COUNT]; /* path list for searching */
    int paths_count;                   /* path count above list */
    char *pattern;
    int worker;
    int omit_threshold;
    int after_context;
    int before_context;
    int context;
    bool file_with_matches;
    bool word_regex;
    bool use_regex;
    bool all_files;
    bool no_omit;
    bool ignore_case;
    bool follow_link;
    bool show_line_number;
    bool color;
    bool group;
    bool buffering;
    bool stdout_redirect;
    bool stdin_redirect;
} hw_option;

extern hw_option op;

void init_option(int argc, char **argv);
void free_option();

#endif // HW_OPTION_H
