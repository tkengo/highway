#include <stdio.h>
#include <string.h>
#include "queue.h"

file_queue *create_file_queue()
{
    file_queue *queue = (file_queue *)malloc(sizeof(file_queue));
    queue->first   = NULL;
    queue->last    = NULL;
    queue->current_for_search = NULL;
    queue->current_for_print = NULL;
    queue->total   = 0;
    return queue;
}

file_queue_node *enqueue_file(file_queue *queue, const char *filename)
{
    file_queue_node *node = (file_queue_node *)malloc(sizeof(file_queue_node));
    node->id          = queue->total++;
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

file_queue_node *dequeue_file_for_search(file_queue *queue)
{
    if (queue->current_for_search) {
        file_queue_node *current_for_search = queue->current_for_search;
        queue->current_for_search = current_for_search->next;
        return current_for_search;
    } else {
        return NULL;
    }
}

file_queue_node *dequeue_string_for_print(file_queue *queue)
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
        free(node);
        node = next;
    }

    free(queue);
}

matched_line_queue *create_matched_line_queue()
{
    matched_line_queue *queue = (matched_line_queue *)malloc(sizeof(matched_line_queue));
    queue->first = NULL;
    queue->last  = NULL;
    return queue;
}

matched_line_queue_node *enqueue_matched_line(matched_line_queue *queue, matched_line_queue_node *node)
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

matched_line_queue_node *dequeue_matched_line(matched_line_queue *queue)
{
    if (queue->first) {
        matched_line_queue_node *first = queue->first;
        queue->first = first->next;
        return first;
    } else {
        return NULL;
    }
}

void free_matched_line_queue(matched_line_queue *queue)
{
    matched_line_queue_node *node = queue->first;
    while (node) {
        matched_line_queue_node *next = node->next;
        free(node->line);
        free(node);
        node = next;
    }

    free(queue);
}

/* file_queue *create_file_queue() */
/* { */
/*     file_queue *queue = (file_queue *)malloc(sizeof(file_queue)); */
/*     queue->first = NULL; */
/*     queue->last  = NULL; */
/*     return queue; */
/* } */
/*  */
/* file_queue_node *enqueue_file(file_queue *queue, char *filename) */
/* { */
/*     file_queue_node *node = (file_queue_node *)malloc(sizeof(file_queue_node)); */
/*     node->next = NULL; */
/*     strcpy(node->filename, filename); */
/*  */
/*     if (queue->first) { */
/*         queue->last->next = node; */
/*         queue->last = node; */
/*     } else { */
/*         queue->first = node; */
/*         queue->last  = node; */
/*     } */
/*  */
/*     return node; */
/* } */
/*  */
/* file_queue_node *dequeue_file(file_queue *queue) */
/* { */
/*     if (queue->first) { */
/*         file_queue_node *first = queue->first; */
/*         queue->first = first->next; */
/*         return first; */
/*     } else { */
/*         return NULL; */
/*     } */
/* } */
