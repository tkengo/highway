#ifndef _FILE_QUEUE_H_
#define _FILE_QUEUE_H_

#include <stdlib.h>
#include "highway.h"
#include "search.h"

typedef struct _file_queue file_queue;
typedef struct _file_queue_node file_queue_node;
typedef struct _matched_line_queue_node matched_line_queue_node;

struct _matched_line_queue_node {
    char *line;
    int line_len;
    int *starts;
    int starts_len;
    matched_line_queue_node *next;
};

typedef struct _line_queue {
    matched_line_queue_node *first;
    matched_line_queue_node *last;
} matched_line_queue;

struct _file_queue_node {
    char filename[256];
    int id;
    file_queue_node *next;
    matched_line_queue *match_lines;
    match *matches;
    bool searched;
};

struct _file_queue {
    file_queue_node *first;
    file_queue_node *last;
    file_queue_node *current_for_search;
    file_queue_node *current_for_print;
    int total;
};

file_queue *create_file_queue();
file_queue_node *enqueue_file(file_queue *queue, char *filename);
file_queue_node *dequeue_file_for_search(file_queue *queue);
file_queue_node *dequeue_string_for_print(file_queue *queue);
void free_file_queue(file_queue *queue);

matched_line_queue *create_matched_line_queue();
matched_line_queue_node *enqueue_matched_line(matched_line_queue *queue, matched_line_queue_node *node);
matched_line_queue_node *dequeue_matched_line(matched_line_queue *queue);
void free_matched_line_queue(matched_line_queue *queue);

#endif // _FILE_QUEUE_H_
