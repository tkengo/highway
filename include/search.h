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
#include "file_queue.h"
#include "option.h"
#include "line_list.h"

bool is_word_match(const char *buf, int len, match *m);
void generate_bad_character_table(const char *pattern, enum file_type t);
int search_by(const char *buf, size_t search_len, const char *pattern, int pattern_len, enum file_type t, match *m, int thread_no);
int format_line(const char *line, int line_len, const char *pattern, int pattern_len, enum file_type t, int line_no, match *first_match, match_line_list *match_line, int thread_no);
int search(int fd, const char *pattern, int pattern_len, enum file_type t, match_line_list *match_line, int thread_no);
int search_stream(const char *pattern);
void free_fjs();

#endif // _HW_TABLE_H_
