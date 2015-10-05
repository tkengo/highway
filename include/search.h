#ifndef _HW_TABLE_H_
#define _HW_TABLE_H_

#define TABLE_SIZE 256
#define N 65536
#define MAX_MATCH_COUNT 1000

enum file_type;

typedef struct _match {
    int start;
    int end;
    int line_no;
    int line_start;
    int line_end;
} match;

#include "file.h"
#include "queue.h"
#include "option.h"

void generate_bad_character_table(const char *pattern, enum file_type t);
int search_(int fd, const char *pattern, enum file_type t, matched_line_queue *match_lines, int *sum_of_actual_match_count);
int search(int fd, const char *pattern, enum file_type t, matched_line_queue *match_line);
int search_stream(const char *pattern);

#endif // _HW_TABLE_H_
