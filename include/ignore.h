#ifndef _HW_IGNORE_H_
#define _HW_IGNORE_H_

#include "common.h"

typedef struct _ignore_list_node ignore_list_node;

struct _ignore_list_node {
    char ignore[1024];
    int base_len;
    bool is_root;
    bool is_dir;
    bool is_no_dir;
    ignore_list_node *next;
};

typedef struct _ignore_list {
    ignore_list_node *first;
    ignore_list_node *last;
    ignore_list_node *acceptable_first;
    ignore_list_node *acceptable_last;
} ignore_list;

ignore_list *create_ignore_list_from_gitignore(const char *path);
ignore_list *create_ignore_list_from_list(const char *path, ignore_list *list);
bool is_ignore(ignore_list *list, const char *filename, const struct dirent *entry);
void free_ignore_list(ignore_list *list);

#endif // _HW_IGNORE_H_
