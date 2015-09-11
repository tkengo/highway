#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include "queue.h"

typedef int bool;

bool is_current_directory(struct dirent *entry)
{
    return entry->d_type == DT_DIR && entry->d_namlen == 1 && entry->d_name[0] == '.';
}

bool is_upper_directory(struct dirent *entry)
{
    return entry->d_type == DT_DIR && entry->d_namlen == 2 && entry->d_name[0] == '.' && entry->d_name[1] == '.';
}

bool is_git_directory(struct dirent *entry)
{
    return entry->d_type == DT_DIR && strcmp(entry->d_name, ".git") == 0;
}

bool is_ignore_directory(struct dirent *entry)
{
    return is_current_directory(entry) || is_upper_directory(entry) || is_git_directory(entry);
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

int main(int argc, char **argv)
{
    file_queue *queue = create_file_queue();
    find_target_files(queue, ".");

    file_queue_node *current;
    while ((current = dequeue_file(queue)) != NULL) {
        printf("%s\n", current->filename);
    }

    return 0;
}
