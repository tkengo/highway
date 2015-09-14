#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include "file.h"
#include "queue.h"

int search(char *filename, char *x)
{
    const int TABLE_SIZE = 256;
    int i, j = 0, m = strlen(x);
    char tbl[TABLE_SIZE];

    FILE *fp = fopen(filename, "r");
    unsigned char y[1024];
    fread(y, sizeof(unsigned char), sizeof(y), fp);
    fclose(fp);
    int n = 1024;

    for (i = 0; i < TABLE_SIZE; ++i) {
        tbl[i] = m + 1;
    }
    for (i = 0; i < m; ++i) {
        tbl[x[i]] = m - i;
    }

    int count = 0;
    char firstCh = x[0];
    char lastCh  = x[m - 1];
    while (j <= n - m) {
        if (lastCh == y[j + m - 1] && firstCh == y[j]) {
            for (i = m - 2; i >= 0 && x[i] == y[j + i]; --i) {
                if (i <= 0) {
                    count++;
                }
            }
        }
        j += tbl[y[j + m]];
    }

    return count;
}

int main(int argc, char **argv)
{
    file_queue *queue = create_file_queue();
    find_target_files(queue, ".");

    file_queue_node *current;
    while ((current = dequeue_file(queue)) != NULL) {
        int count = search(current->filename, argv[1]);
        printf("%s %d\n", current->filename, count);
    }

    return 0;
}
