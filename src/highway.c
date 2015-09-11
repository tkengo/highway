#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include "file.h"
#include "queue.h"

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
