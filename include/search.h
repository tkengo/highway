#ifndef _HW_TABLE_H_
#define _HW_TABLE_H_

#define BAD_CHARACTER_TABLE_SIZE 256
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
int search(int fd, const char *pattern, int pattern_len, enum file_type t, matched_line_queue *match_line, int thread_no);
int search_stream(const char *pattern);
void free_fjs();

#endif // _HW_TABLE_H_
