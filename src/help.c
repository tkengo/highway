#include <stdio.h>

void u(char *p)
{
    printf("%s\n", p);
}

void usage()
{
    u("Usage:");
    u("  hw [OPTIONS] PATTERN");
    u("");
    u("The highway searches a PATTERN from all of files under your directory very fast.");
    u("");
    u("OPTIONS List:");
    u("  -h, --help   Show options help and some concept guides.");
}
