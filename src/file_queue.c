#include <stdio.h>
#include <string.h>
#include "file_queue.h"
#include "hwmalloc.h"

file_queue *create_file_queue()
{
    file_queue *queue = (file_queue *)hw_malloc(sizeof(file_queue));
    queue->first   = NULL;
    queue->last    = NULL;
    queue->current_for_search = NULL;
    return queue;
}

file_queue_node *enqueue_file(file_queue *queue, const char *filename)
{
    file_queue_node *node = (file_queue_node *)hw_malloc(sizeof(file_queue_node));
    node->next        = NULL;
    node->prev        = NULL;
    node->match_lines = NULL;
    node->searched    = false;
    node->matched     = false;
    strcpy(node->filename, filename);

    if (queue->first) {
        node->prev = queue->last;
        queue->last->next = node;
        queue->last = node;

        if (!queue->current_for_search) {
            queue->current_for_search = node;
        }
    } else {
        queue->first   = node;
        queue->last    = node;
        queue->current_for_search = node;
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
