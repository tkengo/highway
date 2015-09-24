#include <stdio.h>

#define u(p) printf("%s\n", p);

void usage()
{
    u("Usage:");
    u("  hw [OPTIONS] PATTERN [PATHS]");
    u("");
    u("The highway searches a PATTERN from all of files under your directory very fast.");
    u("By default hw searches under your current directory, but you can search any");
    u("directories or any files you want to search if you specify the PATHS to the last");
    u("commandline option, and you can specified multiple directories or files to the");
    u("PATHS options.");
    u("");
    u("Example:");
    u("  hw hoge src/ include/ tmp/test.txt");
    u("");
    u("OPTIONS List:");
    u("  -e                       Parse PATTERN as a regular expression.");
    u("  -l, --file-with-matches  Only print filenames that contain matches.");
    u("");
    u("  -h, --help               Show options help and some concept guides.");
}
