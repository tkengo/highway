#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "common.h"
#include "ignore.h"
#include "file.h"
#include "util.h"

ignore_list *create_ignore_list_from_gitignore(char *gitignore)
{
    ignore_list *list = (ignore_list *)malloc(sizeof(ignore_list));
    list->first = NULL;
    list->last  = NULL;

        FILE *fp = fopen(gitignore, "r");
        if (fp != NULL) {
            char ignore[1024];
            while (fgets(ignore, 1024, fp) != NULL) {
                trim(ignore);
                if (ignore[0] == '#' || ignore[0] == '!') {
                    continue;
                }
                add_ignore_list(list, ignore);
            }
            fclose(fp);
        }

    return list;
}

ignore_list_node *add_ignore_list(ignore_list *list, char *ignore)
{
    ignore_list_node *node = (ignore_list_node *)malloc(sizeof(ignore_list_node));

    int len = strlen(ignore);
    node->from_root = *ignore == '/';

    if (*(ignore + len - 1) == '/') {
        *(ignore + len - 1) = '\0';
    }

    strcpy(node->ignore, node->from_root ? ignore + 1 :  ignore);

    if (list->first) {
        list->last->next = node;
        list->last = node;
    } else {
        list->first = node;
        list->last  = node;
    }

    return node;
}

bool is_ignore(ignore_list *list, char *filename)
{
    ignore_list_node *node = list->first;

    while (node) {
        char *index = strstr(filename, node->ignore);
        if (index != NULL) {
            if (node->from_root) {
                return strcmp(filename, node->ignore) == 0;
            } else {
                if (index == filename && strlen(node->ignore) == strlen(index)) {
                    return true;
                }

                if (filename < index && *(index - 1) == '/') {
                    if (strcmp(node->ignore, index) == 0) {
                        return true;
                    }
                }
            }
        }

        node = node->next;
    }

    return false;
}
