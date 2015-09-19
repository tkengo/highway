#ifndef _TABLE_H_
#define _TABLE_H_

#define TABLE_SIZE 256

typedef struct _match {
    int start;
    int line_no;
    int line_start;
} match;

void generate_bad_character_table(char *pattern);
char *get_bad_character_table();
int search(const char *y, int n, const char *x, match *matches);

#endif // _TABLE_H_
