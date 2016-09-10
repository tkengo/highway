#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#ifndef _WIN32
#include <fnmatch.h>
#else
#include <shlwapi.h>
#define fnmatch(x, y, z) (!PathMatchSpec(y, x))
#define index(x, y) strchr(x, y)
#define rindex(x, y) strchr(x, y)
#endif
#include "common.h"
#include "ignore.h"
#include "hwmalloc.h"
#include "file.h"
#include "util.h"

void add_ignore_node(ignore_hash *hash, const char *base, char *pattern, int depth)
{
    ignore_node *node = (ignore_node *)hw_calloc(1, sizeof(ignore_node));

    bool acceptable = pattern[0] == '!';
    if (acceptable) {
        pattern++;
    }

    node->is_root = IS_PATHSEP(pattern[0]);
    if (node->is_root) {
        pattern++;
    }

    int last_index = strlen(pattern) - 1;
    if (last_index == 0) {
        return;
    }

    node->depth = depth;
    node->is_dir = IS_PATHSEP(pattern[last_index]);
    if (node->is_dir) {
        pattern[last_index] = '\0';
    }
#ifndef _WIN32
    node->is_no_dir = index(pattern, '/') == NULL;
#else
    node->is_no_dir = index(pattern, '/') == NULL && index(pattern, '\\') == NULL;
#endif

    ignore_node **first;
    if (acceptable) {
        // This is acceptable pattern.
        strcpy(node->name, pattern);
        first = &hash->accept;
    } else if (pattern[0] == '*' && pattern[1] == '.' && strpbrk(pattern + 2, ".?*[]") == NULL) {
        // If a pattern was matched above condition, it represents the "extention". For instance:
        // *.c, *.cpp, *.h, and so on...
        strcpy(node->name, pattern + 2);
        first = &hash->ext[(unsigned char)node->name[0]];
    } else if (strpbrk(pattern, "?*[]") != NULL || (!node->is_root && !node->is_no_dir)) {
        // This is handled as a glob format, so fnmatch is used in matching test. For instance:
        // test*, src/test.c, and so on ...
        strcpy(node->name, pattern);
        first = &hash->glob;
    } else {
        if (node->is_root) {
            sprintf(node->name, "%s%s", base, pattern);
        } else {
            strcpy(node->name, pattern);
        }
        first = &hash->path[(unsigned char)node->name[0]];
    }

    node->next = *first;
    *first = node;
}

ignore_hash *merge_ignore_hash(ignore_hash *hash, const char *base, const char *path, int depth)
{
    FILE *fp = fopen(path, "r");
    if (!fp) {
        return hash;
    }

    if (hash == NULL) {
        ignore_hash *new_hash = (ignore_hash *)hw_calloc(1, sizeof(ignore_hash));
        hash = new_hash;
    }

    char buf[MAX_PATH_LENGTH];
    while (fgets(buf, MAX_PATH_LENGTH, fp) != NULL) {
        trim(buf);
        if (buf[0] == '#' || strlen(buf) == 0 || buf[0] == '\n' || buf[0] == '\r') {
            continue;
        }
        add_ignore_node(hash, base, buf, depth);
    }
    fclose(fp);

    return hash;
}

/**
 * Load ignore list from the .gitignore file in the specified path. Return NULL if there is no
 * .gitignore file.
 */
ignore_hash *load_ignore_hash(const char *base, const char *path, int depth)
{
    return merge_ignore_hash(NULL, base, path, depth);
}

bool match_path(ignore_node *node, const char *path, bool base_is_dir, const struct dirent *entry)
{
    while (node) {
        if (node->is_dir && !base_is_dir) {
            node = node->next;
            continue;
        }

        int res;
        if (node->is_root) {
            res = fnmatch(node->name, path, FNM_PATHNAME);
        } else if (node->is_no_dir) {
            res = fnmatch(node->name, entry->d_name, 0);
        } else {
            res = strstr(path + node->base_len, node->name) != NULL ?
                  0 :
                  fnmatch(node->name, path + node->base_len + 1, 0);
        }

        if (res == 0) {
            return true;
        }

        node = node->next;
    }

    return false;
}

bool is_ignore(ignore_hash *hash, const char *path, bool is_dir, const struct dirent *entry)
{
    if (match_path(hash->accept, path, is_dir, entry)) {
        return false;
    }

    ignore_node *node;

    if (!is_dir) {
        char *ext = rindex(path, '.');
        if (ext && ext[1] != '\0') {
            ext++;
            node = hash->ext[(unsigned char)ext[0]];
            while (node) {
                if (strcmp(ext, node->name) == 0) {
                    return true;
                }
                node = node->next;
            }
        }
    }

    node = hash->path[(unsigned char)path[0]];
    while (node) {
        bool is_skip = (node->is_dir && !is_dir) ||
                       !node->is_root;
        if (is_skip) {
            node = node->next;
            continue;
        }

        if (strcmp(path, node->name) == 0) {
            return true;
        }
        node = node->next;
    }

    node = hash->path[(unsigned char)entry->d_name[0]];
    while (node) {
        bool is_skip = (node->is_dir && !is_dir) ||
                       !node->is_no_dir ||
                       node->is_root;
        if (is_skip) {
            node = node->next;
            continue;
        }

        if (strcmp(entry->d_name, node->name) == 0) {
            return true;
        }
        node = node->next;
    }

    return match_path(hash->glob, path, is_dir, entry);
}

ignore_node *free_ignore_hash_by_depth(ignore_node *node, int depth)
{
    while (node) {
        if (node->depth < depth) {
            break;
        }
        ignore_node *unnecessary = node;
        node = node->next;
        tc_free(unnecessary);
    }
    return node;
}

void free_ignore_hash(ignore_hash *hash, int depth)
{
    for (int i = 0; i < IGNORE_TABLE_SIZE; i++) {
        hash->ext[i] = free_ignore_hash_by_depth(hash->ext[i], depth);
    }

    for (int i = 0; i < IGNORE_TABLE_SIZE; i++) {
        hash->path[i] = free_ignore_hash_by_depth(hash->path[i], depth);
    }

    hash->glob   = free_ignore_hash_by_depth(hash->glob, depth);
    hash->accept = free_ignore_hash_by_depth(hash->accept, depth);
}
