#include <stdio.h>

void u(char *p)
{
    printf("%s\n", p);
}

void usage()
{
    u("Usage:");
    u("  hw [OPTIONS] PATTERN [PATH]");
    u("");
    u("The highway searches a PATTERN from all of files under your directory very fast.");
    u("By default hw searches under your current directory, but you can search any directories");
    u("you want to search if you specify the PATH to the last commandline option.");
    u("");
    u("OPTIONS List:");
    u("  -h, --help   Show options help and some concept guides.");
}
