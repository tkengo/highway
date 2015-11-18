#ifndef HW_TABLE_H
#define HW_TABLE_H

#define BAD_CHARACTER_TABLE_SIZE 256
#define NMAX 65536
#define MAX_MATCH_COUNT 5000

enum file_type;

typedef struct _match {
    int start;
    int end;
    int line_no;
    int line_start;
    int line_end;
} match;

enum append_type {
    AT_FIRST = 0,
    AT_MIDDLE,
    AT_LAST
};

#include "file.h"
#include "file_queue.h"
#include "option.h"
#include "line_list.h"

bool is_word_match(const char *buf, int len, match *m);
void generate_bad_character_table(const char *pattern, enum file_type t);
int search(int fd, file_queue_node *current, const char *pattern, int pattern_len, enum file_type t, match_line_list *match_lines, int thread_no);

#endif // HW_TABLE_H
