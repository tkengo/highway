#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "common.h"
#include "highway.h"
#include "option.h"
#include "log.h"
#include "util.h"
#include "help.h"

hw_option op;

void init_option(int argc, char **argv, hw_option *op)
{
    bool help = false;
    static int flag;

    static struct option longopts[] = {
        { "all-files",         no_argument,       NULL,  'a' },
        { "follow-link",       no_argument,       NULL,  'f' },
        { "help",              no_argument,       NULL,  'h' },
        { "ignore-case",       no_argument,       NULL,  'i' },
        { "file-with-matches", no_argument,       NULL,  'l' },
        { "word-regexp",       no_argument,       NULL,  'w' },
        { "debug",             no_argument,       &flag, 1   },
        { "worker",            required_argument, &flag, 2   },
        { "no-omit",           no_argument,       &flag, 3   },
        { 0, 0, 0, 0 }
    };

    op->worker            = DEFAULT_WORKER;
    op->root_paths[0]     = ".";
    op->paths_count       = 1;
    op->file_with_matches = false;
    op->use_regex         = false;
    op->all_files         = false;
    op->no_omit           = false;
    op->ignore_case       = false;
    op->follow_link       = false;

    int ch;
    bool word_regex = false;
    while ((ch = getopt_long(argc, argv, "aefhilw", longopts, NULL)) != -1) {
        switch (ch) {
            case 0:
                switch (flag) {
                    case 1: /* --debug */
                        set_log_level(LOG_LEVEL_DEBUG);
                        break;
                    case 2: /* --worker */
                        op->worker = atoi(optarg);
                        break;
                    case 3: /* --no-omit */
                        op->no_omit = true;
                        break;
                }
                break;

            case 'a': /* All files searching */
                op->all_files = true;
                break;

            case 'e': /* Use regular expression */
                op->use_regex = true;
                break;

            case 'f': /* Following symbolic link */
                op->follow_link = true;
                break;

            case 'h': /* Show help */
                usage();
                exit(0);
                break;

            case 'i': /* Ignore case */
                op->ignore_case = true;
                op->use_regex   = true;
                break;

            case 'l': /* Show only filenames */
                op->file_with_matches = true;
                break;

            case 'w': /* Match only whole word */
                word_regex = true;
                break;

            case '?':
            default:
                usage();
                exit(1);
        }
    }

    if (argc == optind) {
        usage();
        exit(1);
    }

    char *pattern = argv[optind++];
    if (word_regex) {
        op->pattern = (char *)malloc(sizeof(char) * (strlen(pattern) + 5));
        sprintf(op->pattern, "\\b%s\\b", pattern);
        op->use_regex = true;
    } else {
        op->pattern = (char *)malloc(sizeof(char) * (strlen(pattern) + 1));
        strcpy(op->pattern, pattern);
    }

    op->pattern_len = strlen(op->pattern);
    int paths_count = argc - optind;
    if (paths_count > MAX_PATHS_COUNT) {
        log_buffered("Too many PATHs(%d) was passed. You can specify %d paths at most.", paths_count, MAX_PATHS_COUNT);
        log_buffered("You might want to enclose PATHs by quotation If you use glob format.", paths_count, MAX_PATHS_COUNT);
        paths_count = MAX_PATHS_COUNT;
    }
    if (paths_count > 0) {
        for (int i = 0; i < paths_count; i++) {
            char *path = argv[optind + i];
            int len = strlen(path);
            if (path[len - 1] == '/') {
                path[len - 1] = '\0';
            }
            op->root_paths[i] = path;
        }
        op->paths_count = paths_count;
    }
}

void free_option(hw_option *op)
{
    free(op->pattern);
}
