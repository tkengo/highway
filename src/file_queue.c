#include <stdio.h>
#include <string.h>
#include <gperftools/tcmalloc.h>
#include "file_queue.h"

file_queue *create_file_queue()
{
    file_queue *queue = (file_queue *)tc_malloc(sizeof(file_queue));
    queue->first   = NULL;
    queue->last    = NULL;
    queue->current_for_search = NULL;
    queue->current_for_print = NULL;
    queue->total   = 0;
    return queue;
}

file_queue_node *enqueue_file(file_queue *queue, const char *filename)
{
    file_queue_node *node = (file_queue_node *)tc_malloc(sizeof(file_queue_node));
    node->next        = NULL;
    node->match_lines = NULL;
    node->searched    = false;
    node->matched     = false;
    strcpy(node->filename, filename);

    if (queue->first) {
        queue->last->next = node;
        queue->last = node;

        if (!queue->current_for_search) {
            queue->current_for_search = node;
        }
        if (!queue->current_for_print) {
            queue->current_for_print = node;
        }
    } else {
        queue->first   = node;
        queue->last    = node;
        queue->current_for_search = node;
        queue->current_for_print = node;
    }

    return node;
}

file_queue_node *peek_file_for_search(file_queue *queue)
{
    if (queue->current_for_search) {
        file_queue_node *current_for_search = queue->current_for_search;
        queue->current_for_search = current_for_search->next;
        return current_for_search;
    } else {
        return NULL;
    }
}

file_queue_node *peek_file_for_print(file_queue *queue)
{
    if (queue->current_for_print) {
        file_queue_node *current_for_print = queue->current_for_print;
        queue->current_for_print = current_for_print->next;
        return current_for_print;
    } else {
        return NULL;
    }
}

void free_file_queue(file_queue *queue)
{
    file_queue_node *node = queue->first;
    while (node) {
        file_queue_node *next = node->next;
        tc_free(node);
        node = next;
    }

    tc_free(queue);
}
