#ifndef HW_LINE_LIST_H
#define HW_LINE_LIST_H

typedef struct _match_line_list match_line_list;
typedef struct _match_line_node match_line_node;

enum context_type {
    CONTEXT_NONE = 0,
    CONTEXT_BEFORE,
    CONTEXT_AFTER
};

struct _match_line_node {
    char *line;
    int line_no;
    enum context_type context;
    match_line_node *next;
};

struct _match_line_list {
    match_line_node *first;
    match_line_node *last;
    match_line_node *current_for_search;
    int max_line_no;
};

match_line_list *create_match_line_list();
match_line_node *enqueue_match_line(match_line_list *list, match_line_node *node);
match_line_node *dequeue_match_line(match_line_list *list);
void clear_line_list(match_line_list *list);
void free_match_line_list(match_line_list *list);

#endif // HW_LINE_LIST_H
