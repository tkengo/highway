#ifndef _TABLE_H_
#define _TABLE_H_

#define TABLE_SIZE 256
#define N 65535
#define MAX_MATCH_COUNT 100

typedef struct _match {
    int start;
    int line_no;
    int line_start;
} match;

#include "queue.h"

void generate_bad_character_table(char *pattern);
char *get_bad_character_table();
int search(int fd, char *buf, char *pattern, matched_line_queue *match_lines);
int ssabs(const unsigned char *buf, int buf_len, const char *pattern, match *matches, int max_match);

#endif // _TABLE_H_
