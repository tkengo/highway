#include <stdio.h>

#define u(p) printf("%s\n", p);

void usage()
{
    u("Usage:");
    u("  hw [OPTIONS] PATTERN [PATHS]");
    u("");
    u("The highway searches a PATTERN from all of files under your directory very fast.");
    u("By default hw searches in under your current directory except hidden files and");
    u("paths from .gitignore, but you can search any directories or any files you want");
    u("to search by specifying the PATHS, and you can specified multiple directories or");
    u("files to the PATHS options.");
    u("");
    u("Example:");
    u("  hw hoge src/ include/ tmp/test.txt");
    u("");
    u("OPTIONS List:");
    u("  -a, --all-files          Search all files.");
    u("  -e                       Parse PATTERN as a regular expression.");
    u("  -i, --ignore-case        Match case insensitively.");
    u("  -l, --file-with-matches  Only print filenames that contain matches.");
    u("      --no-omit            Show all matches even if too many matches was found.");
    u("                           You don't have to use this option usually.");
    u("  -w, --word-regexp        Only match whole words.");
    u("");
    u("  -h, --help               Show options help and some concept guides.");
}
