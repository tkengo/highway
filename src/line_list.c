#include "line_list.h"
#include "hwmalloc.h"

match_line_list *create_match_line_list()
{
    match_line_list *list = (match_line_list *)hw_malloc(sizeof(match_line_list));
    list->first = NULL;
    list->last  = NULL;
    list->current_for_search = NULL;
    list->max_line_no = 1;
    return list;
}

match_line_node *enqueue_match_line(match_line_list *list, match_line_node *node)
{
    node->next = NULL;

    if (list->first) {
        list->last->next = node;
        list->last = node;
    } else {
        list->first = node;
        list->last  = node;
        list->current_for_search = node;
    }

    return node;
}

match_line_node *dequeue_match_line(match_line_list *list)
{
    if (list->current_for_search) {
        match_line_node *current_for_search = list->current_for_search;
        list->current_for_search = current_for_search->next;
        return current_for_search;
    } else {
        return NULL;
    }
}

void clear_line_list(match_line_list *list) {
    match_line_node *node = list->first;
    while (node) {
        match_line_node *next = node->next;
        tc_free(node->line);
        tc_free(node);
        node = next;
    }

    list->first = NULL;
    list->last  = NULL;
    list->current_for_search = NULL;
    list->max_line_no = 1;
}

void free_match_line_list(match_line_list *list)
{
    clear_line_list(list);
    tc_free(list);
}
