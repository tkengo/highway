#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include "file.h"

bool is_ignore_directory(struct dirent *entry)
{
    bool cur = entry->d_type == DT_DIR && entry->d_namlen == 1 && entry->d_name[0] == '.';
    bool up  = entry->d_type == DT_DIR && entry->d_namlen == 2 && entry->d_name[0] == '.' && entry->d_name[1] == '.';
    bool git = entry->d_type == DT_DIR && strcmp(entry->d_name, ".git") == 0;

    return cur || up || git;
}

bool is_directory(struct dirent *entry)
{
    return entry->d_type == DT_DIR;
}

bool is_search_target(struct dirent *entry)
{
    return entry->d_type == DT_REG || entry->d_type == DT_LNK;
}

void find_target_files(file_queue *queue, char *dirname)
{
    DIR *dir = opendir(dirname);
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (is_ignore_directory(entry)) {
            continue;
        }

        char buf[256];
        sprintf(buf, "%s/%s", dirname, entry->d_name);

        if (is_directory(entry)) {
            find_target_files(queue, buf);
        } else if (is_search_target(entry)) {
            enqueue_file(queue, buf);
        }
    }
    closedir(dir);
}
