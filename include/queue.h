#ifndef _FILE_QUEUE_H_
#define _FILE_QUEUE_H_

#include <stdlib.h>

typedef struct _file_queue_node {
    char filename[256];
    int id;
    struct _file_queue_node *next;
} file_queue_node;

typedef struct _file_queue {
    file_queue_node *first;
    file_queue_node *last;
    int total;
} file_queue;

file_queue *create_file_queue();
file_queue_node *enqueue_file(file_queue *queue, char *filename);
file_queue_node *dequeue_file(file_queue *queue);

#endif // _FILE_QUEUE_H_
