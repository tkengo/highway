#include "line_list.h"
#include "hwmalloc.h"

match_line_list *create_match_line_list()
{
    match_line_list *queue = (match_line_list *)tc_malloc(sizeof(match_line_list));
    queue->first = NULL;
    queue->last  = NULL;
    return queue;
}

match_line_node *enqueue_match_line(match_line_list *queue, match_line_node *node)
{
    node->next = NULL;

    if (queue->first) {
        queue->last->next = node;
        queue->last = node;
    } else {
        queue->first = node;
        queue->last  = node;
    }

    return node;
}

match_line_node *dequeue_match_line(match_line_list *queue)
{
    if (queue->first) {
        match_line_node *first = queue->first;
        queue->first = first->next;
        return first;
    } else {
        return NULL;
    }
}

void free_match_line_list(match_line_list *queue)
{
    match_line_node *node = queue->first;
    while (node) {
        match_line_node *next = node->next;
        tc_free(node->line);
        tc_free(node);
        node = next;
    }

    tc_free(queue);
}
