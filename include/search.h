#ifndef _HW_TABLE_H_
#define _HW_TABLE_H_

#define TABLE_SIZE 256
#define N 65536
#define MAX_MATCH_COUNT 100

#include "option.h"

typedef struct _match {
    int start;
    int line_no;
    int line_start;
} match;

#include "queue.h"

void generate_bad_character_table(char *pattern);
int search(int fd, hw_option *op, matched_line_queue *match_lines);

#endif // _HW_TABLE_H_
