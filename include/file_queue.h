#ifndef _HW_FILE_QUEUE_H_
#define _HW_FILE_QUEUE_H_

#include <stdlib.h>
#include "common.h"

typedef struct _file_queue file_queue;
typedef struct _file_queue_node file_queue_node;

#include "search.h"
#include "file.h"
#include "line_list.h"

struct _file_queue_node {
    char filename[MAX_PATH_LENGTH];
    file_queue_node *next;
    file_queue_node *prev;
    match_line_list *match_lines;
    bool searched;
    bool matched;
    enum file_type t;
};

struct _file_queue {
    file_queue_node *first;
    file_queue_node *last;
    file_queue_node *current_for_search;
};

file_queue *create_file_queue();
file_queue_node *enqueue_file(file_queue *queue, const char *filename);
file_queue_node *peek_file_for_search(file_queue *queue);

#endif // _HW_FILE_QUEUE_H_
