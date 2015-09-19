#ifndef _OPTION_H_
#define _OPTION_H_

typedef struct _hw_option {
    char *pattern;
    int worker;
} hw_option;

void init_option(int argc, char **argv, hw_option *op);

#endif // _OPTION_H_
