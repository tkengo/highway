#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include "common.h"
#include "ignore.h"
#include "file.h"
#include "util.h"

ignore_list_node *add_ignore_list(ignore_list *list, const char *base, char *ignore)
{
    bool acceptable = ignore[0] == '!';
    if (acceptable) {
        ignore++;
    }

    ignore_list_node *node = (ignore_list_node *)malloc(sizeof(ignore_list_node));

    node->next = NULL;

    node->is_root = *ignore == '/';
    node->base_len = strlen(base);

    int len = strlen(ignore);
    if (ignore[len - 1] == '/') {
        node->is_dir = true;
        ignore[len - 1] = '\0';
    }
    if (strlen(ignore) == 0) {
        return NULL;
    }

    node->is_no_dir = index(ignore, '/') == NULL;

    if (node->is_root) {
        sprintf(node->ignore, "%s%s", base, ignore);
    } else {
        strcpy(node->ignore, ignore);
    }

    if (acceptable) {
        if (list->acceptable_first) {
            list->acceptable_last->next = node;
            list->acceptable_last = node;
        } else {
            list->acceptable_first = node;
            list->acceptable_last  = node;
        }
    } else {
        if (list->first) {
            list->last->next = node;
            list->last = node;
        } else {
            list->first = node;
            list->last  = node;
        }
    }

    return node;
}

/**
 * Create ignore list from the .gitignore file in the specified path. Return NULL if there is no
 * the .gitignore file.
 */
ignore_list *create_ignore_list_from_gitignore(const char *base, const char *path)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fclose(fp);
        return NULL;
    }

    ignore_list *list = (ignore_list *)malloc(sizeof(ignore_list));
    list->first = NULL;
    list->last  = NULL;
    list->acceptable_first = NULL;
    list->acceptable_last  = NULL;

    char buf[1024];
    int count = 0;
    while (fgets(buf, 1024, fp) != NULL) {
        trim(buf);
        if (buf[0] == '#' || strlen(buf) == 0) {
            continue;
        }
        add_ignore_list(list, base, buf);
        count++;
    }
    fclose(fp);

    if (count == 0) {
        free(list);
        return NULL;
    }

    return list;
}

ignore_list *create_ignore_list_from_list(const char *base, const char *path, ignore_list *list)
{
    ignore_list *new_list = create_ignore_list_from_gitignore(base, path);
    if (new_list == NULL) {
        return list;
    }

    ignore_list_node *node = list->first;
    while (node) {
        if (!node->is_root) {
            ignore_list_node *new_node = (ignore_list_node *)malloc(sizeof(ignore_list_node));
            *new_node = *node;
            new_node->next = NULL;
            new_list->last->next = new_node;
            new_list->last = new_node;
        }
        node = node->next;
    }

    return new_list;
}

bool match_path(ignore_list_node *node, const char *path, const struct dirent *entry)
{
    while (node) {
        if (node->is_dir && !is_directory(entry)) {
            node = node->next;
            continue;
        }

        int res;
        if (node->is_root) {
            res = fnmatch(node->ignore, path, FNM_PATHNAME);
        } else if (node->is_no_dir) {
            res = fnmatch(node->ignore, entry->d_name, 0);
        } else {
            res = strstr(path + node->base_len, node->ignore) != NULL ?
                  0 :
                  fnmatch(node->ignore, path + node->base_len + 1, 0);
        }

        if (res == 0) {
            return true;
        }

        node = node->next;
    }

    return false;
}

bool is_ignore(ignore_list *list, const char *path, const struct dirent *entry)
{
    if (match_path(list->acceptable_first, path, entry)) {
        return false;
    }

    return match_path(list->first, path, entry);
}

void free_ignore_list(ignore_list *list)
{
    ignore_list_node *node = list->first;
    while (node) {
        ignore_list_node *next = node->next;
        free(node);
        node = next;
    }

    free(list);
}
