#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <fnmatch.h>
#include "common.h"
#include "ignore.h"
#include "file.h"
#include "util.h"

/**
 * Create ignore list from the .gitignore file in the specified path. Return NULL if there is no
 * the .gitignore file.
 */
ignore_list *create_ignore_list_from_gitignore(const char *path)
{
    char buf[1024];
    sprintf(buf, "%s/%s", path, ".gitignore");

    FILE *fp = fopen(buf, "r");
    if (!fp) {
        fclose(fp);
        return NULL;
    }

    ignore_list *list = (ignore_list *)malloc(sizeof(ignore_list));
    list->first = NULL;
    list->last  = NULL;

    while (fgets(buf, 1024, fp) != NULL) {
        trim(buf);
        if (buf[0] == '#' || buf[0] == '!') {
            continue;
        }
        add_ignore_list(list, buf);
    }
    fclose(fp);

    return list;
}

ignore_list_node *add_ignore_list(ignore_list *list, char *ignore)
{
    ignore_list_node *node = (ignore_list_node *)malloc(sizeof(ignore_list_node));

    int len = strlen(ignore);
    node->is_root = *ignore == '/';

    if (*(ignore + len - 1) == '/') {
        node->is_dir = true;
        *(ignore + len - 1) = '\0';
    }
    node->is_no_dir = index(ignore, '/') == NULL;

    strcpy(node->ignore, ignore);

    if (list->first) {
        list->last->next = node;
        list->last = node;
    } else {
        list->first = node;
        list->last  = node;
    }

    return node;
}

bool is_ignore(ignore_list *list, const char *base, const char *filename, const struct dirent *entry)
{
    ignore_list_node *node = list->first;

    while (node) {
        if (node->is_dir && !is_directory(entry)) {
            node = node->next;
            continue;
        }

        int res;
        if (node->is_root) {
            res = fnmatch(node->ignore, filename + strlen(base), FNM_PATHNAME);
        } else if (node->is_no_dir) {
            res = fnmatch(node->ignore, entry->d_name, 0);
        } else {
            res = fnmatch(node->ignore, filename + strlen(base), 0);
        }

        if (res == 0) {
            return true;
        }

        node = node->next;
    }

    return false;
}
