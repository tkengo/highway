#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "hwmalloc.h"
#include "config.h"
#include "common.h"
#include "highway.h"
#include "option.h"
#include "log.h"
#include "util.h"
#include "help.h"

hw_option op;

void init_option(int argc, char **argv)
{
    static int flag;

    static struct option longopts[] = {
        { "all-files",         no_argument,       NULL,  'a' },
        { "follow-link",       no_argument,       NULL,  'f' },
        { "help",              no_argument,       NULL,  'h' },
        { "ignore-case",       no_argument,       NULL,  'i' },
        { "file-with-matches", no_argument,       NULL,  'l' },
        { "line-number",       no_argument,       NULL,  'n' },
        { "word-regexp",       no_argument,       NULL,  'w' },
        { "after-context",     required_argument, NULL,  'A' },
        { "before-context",    required_argument, NULL,  'B' },
        { "context",           required_argument, NULL,  'C' },
        { "no-line-number",    no_argument,       NULL,  'N' },
        { "debug",             no_argument,       &flag, 1   },
        { "worker",            required_argument, &flag, 2   },
        { "no-omit",           no_argument,       &flag, 3   },
        { "version",           no_argument,       &flag, 4   },
        { "color",             no_argument,       &flag, 5   },
        { "no-color",          no_argument,       &flag, 6   },
        { "group",             no_argument,       &flag, 7   },
        { "no-group",          no_argument,       &flag, 8   },
        { 0, 0, 0, 0 }
    };

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    op.worker            = MAX(DEFAULT_WORKER, sysconf(_SC_NPROCESSORS_ONLN) - 1);
    op.root_paths[0]     = ".";
    op.paths_count       = 1;
    op.omit_threshold    = MAX(MIN_LINE_LENGTH, w.ws_col / 2);
    op.after_context     = 0;
    op.before_context    = 0;
    op.context           = 0;
    op.file_with_matches = false;
    op.word_regex        = false;
    op.use_regex         = false;
    op.all_files         = false;
    op.no_omit           = false;
    op.ignore_case       = false;
    op.follow_link       = false;
    op.stdout_redirect   = IS_STDOUT_REDIRECT;
    op.stdin_redirect    = IS_STDIN_REDIRECT;
    op.show_line_number  = !op.stdin_redirect;
    op.color             = !op.stdout_redirect;
    op.group             = !op.stdout_redirect && !op.stdin_redirect;

    int ch;
    bool show_version = false;
    while ((ch = getopt_long(argc, argv, "aefhilnvwA:B:C:N", longopts, NULL)) != -1) {
        switch (ch) {
            case 0:
                switch (flag) {
                    case 1: /* --debug */
                        set_log_level(LOG_LEVEL_DEBUG);
                        break;
                    case 2: /* --worker */
                        op.worker = atoi(optarg);
                        break;
                    case 3: /* --no-omit */
                        op.no_omit = true;
                        break;
                    case 4: /* --version */
                        show_version = true;
                        break;
                    case 5: /* --color */
                        op.color = true;
                        break;
                    case 6: /* --no-color */
                        op.color = false;
                        break;
                    case 7: /* --group */
                        op.group = true;
                        break;
                    case 8: /* --no-group */
                        op.group = false;
                        break;
                }
                break;

            case 'a': /* All files searching */
                op.all_files = true;
                break;

            case 'e': /* Use regular expression */
                op.use_regex = true;
                break;

            case 'f': /* Following symbolic link */
                op.follow_link = true;
                break;

            case 'h': /* Show help */
                usage();
                exit(0);
                break;

            case 'i': /* Ignore case */
                op.ignore_case = true;
                op.use_regex   = true;
                break;

            case 'l': /* Show only filenames */
                op.file_with_matches = true;
                break;

            case 'n': /* Show line number */
                op.show_line_number = true;
                break;

            case 'w': /* Match only whole word */
                op.word_regex = true;
                break;

            case 'A': /* After context */
                op.after_context = atoi(optarg);
                break;

            case 'B': /* Before context */
                op.before_context = atoi(optarg);
                break;

            case 'C': /* Context */
                op.context = atoi(optarg);
                break;

            case 'N': /* Not show line number */
                op.show_line_number = false;
                break;

            case '?':
            default:
                usage();
                exit(1);
        }
    }

    if (show_version) {
        printf("highway version %s\n", PACKAGE_VERSION);
        exit(0);
    }

    if (argc == optind) {
        usage();
        exit(1);
    }

    char *pattern = argv[optind++];
    op.pattern = (char *)tc_malloc(SIZE_OF_CHAR * (strlen(pattern) + 1));
    strcpy(op.pattern, pattern);

    int paths_count = argc - optind;
    if (paths_count > MAX_PATHS_COUNT) {
        log_w("Too many PATHs(%d) was passed. You can specify %d paths at most.", paths_count, MAX_PATHS_COUNT);
        log_w("You might want to enclose PATHs by quotation If you use glob format.");
        paths_count = MAX_PATHS_COUNT;
    }
    if (paths_count > 0) {
        for (int i = 0; i < paths_count; i++) {
            char *path = argv[optind + i];
            int len = strlen(path);
            if (path[len - 1] == '/') {
                path[len - 1] = '\0';
            }
            op.root_paths[i] = path;
        }
        op.paths_count = paths_count;
    }
}

void free_option()
{
    tc_free(op.pattern);
}
