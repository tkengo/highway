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
    u("Search options:");
    u("  -a, --all-files          Search all files.");
    u("  -e                       Parse PATTERN as a regular expression.");
    u("  -f, --follow-link        Follow symlinks.");
    u("  -i, --ignore-case        Match case insensitively.");
    u("  -l, --file-with-matches  Only print filenames that contain matches.");
    u("  -w, --word-regexp        Only match whole words.");
    u("");
    u("Output options:");
    u("  -n, --line-number        Print line number with output lines.");
    u("  -N, --no-line-number     Don't print line number.");
    u("      --no-omit            Show all characters even if too long lines were matched.");
    u("                           By default hw print only characters near by PATTERN if");
    u("                           the line was too long.");
    u("");
    u("Context control:");
    u("  -A, --after-context NUM  Print NUM lines after match.");
    u("  -B, --before-context NUM Print NUM lines before match.");
    u("  -C, --context NUM        Print NUM lines before and after matches.");
    u("");
    u("  -h, --help               Show options help and some concept guides.");
    u("      --version            Show version.");
}
