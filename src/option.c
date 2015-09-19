#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include "highway.h"
#include "option.h"
#include "log.h"
#include "help.h"

void init_option(int argc, char **argv, hw_option *op)
{
    bool help = false;
    static int debug;

    static struct option longopts[] = {
        { "help",  no_argument, NULL,   'h' },
        { "debug", no_argument, &debug, 1   },
        { 0,       0,           0,      0   }
    };

    int ch;
    while ((ch = getopt_long(argc, argv, "h", longopts, NULL)) != -1) {
        switch (ch) {
            case 0:
                break;
            case 'h':
                usage();
                exit(0);
                break;

            case '?':
            default:
                usage();
                exit(1);
        }
    }

    if (debug) {
        set_log_level(LOG_LEVEL_DEBUG);
    }

    if (argc == optind) {
        usage();
        exit(1);
    }

    op->pattern = argv[optind];
}
