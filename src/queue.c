#include "queue.h"

file_queue *create_file_queue()
{
    file_queue *queue = (file_queue *)malloc(sizeof(file_queue));
    queue->first = NULL;
    queue->last  = NULL;
    return queue;
}

file_queue_node *enqueue_file(file_queue *queue, char *filename)
{
    file_queue_node *node = (file_queue_node *)malloc(sizeof(file_queue_node));
    node->next = NULL;
    node->filename = filename;

    if (queue->first) {
        queue->first = node;
        queue->last  = node;
    } else {
        queue->last->next = node;
        queue->last = node;
    }

    return node;
}

file_queue_node *dequeue_file(file_queue *queue)
{
    if (queue->first) {
        file_queue_node *first = queue->first;
        queue->first = first->next;
        return first;
    } else {
        return NULL;
    }
}
