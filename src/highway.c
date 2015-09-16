#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "highway.h"
#include "file.h"
#include "queue.h"

#define TABLE_SIZE 256
#define FILENAME_COLOR "\033[1;32m"
#define RESET_COLOR "\033[0m\033[K"
#define N 65535

char tbl[TABLE_SIZE];
char *pattern = "include";
unsigned char *y;

int search(const unsigned char *y, int n, const char *x, int *matches)
{
    int i, j = 0, m = strlen(x);

    int count = 0;
    char firstCh = x[0];
    char lastCh  = x[m - 1];
    while (j <= n - m) {
        if (lastCh == y[j + m - 1] && firstCh == y[j]) {
            for (i = m - 2; i >= 0 && x[i] == y[j + i]; --i) {
                if (i <= 0) {
                    // matched
                    count++;
                }
            }
        }
        j += tbl[y[j + m]];
    }

    return count;
}

file_queue *queue;
pthread_mutex_t mutex;
pthread_cond_t queue_cond;
bool hoge = false;

void print_result(const unsigned char *buf, const char *filename)
{
    printf("%s%s%s\n", FILENAME_COLOR, filename, RESET_COLOR);
}

void *worker(void *arg)
{
    int id = *((int *)arg);
    unsigned char *buf = (unsigned char *)malloc(sizeof(unsigned char) * N);

    file_queue_node *current;
    /* printf("%dth worker was started\n", id); */
    while (1) {
        pthread_mutex_lock(&mutex);
        current = dequeue_file(queue);
        if (current == NULL) {
            if (hoge) {
                pthread_mutex_unlock(&mutex);
                /* printf("%dth workor die\n", id); */
                break;
            }
            pthread_cond_wait(&queue_cond, &mutex);
        }
        current = dequeue_file(queue);
        pthread_mutex_unlock(&mutex);

        if (current != NULL) {
            int fd = open(current->filename, O_RDONLY);
            if (!is_binary(fd)) {
                int count = 0;
                int n = read(fd, buf, N);
                int *matches;
                count = search(buf, n, pattern, matches);
                if (count > 0) {
                    print_result(buf, current->filename);
                }
            }

            close(fd);
        }
    }

    free(buf);

    return NULL;
}

void find_target_files2(file_queue *queue, char *dirname)
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
            pthread_cond_signal(&queue_cond);
        }
    }
    closedir(dir);
}

int main(int argc, char **argv)
{
    queue = create_file_queue();

    int i, m = strlen(pattern);
    for (i = 0; i < TABLE_SIZE; ++i) {
        tbl[i] = m + 1;
    }
    for (i = 0; i < m; ++i) {
        tbl[pattern[i]] = m - i;
    }

    pthread_cond_init(&queue_cond, NULL);
    pthread_mutex_init(&mutex, NULL);
    y = (unsigned char *)malloc(sizeof(unsigned char) * N);
    const int THREAD_COUNT = atoi(argv[1]);
    int *id = (int *)malloc(THREAD_COUNT * sizeof(int));
    pthread_t th[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        id[i] = i;
        pthread_create(&th[i], NULL, (void *)worker, (void *)&id[i]);
    }

    find_target_files2(queue, ".");

    hoge = true;
    pthread_cond_broadcast(&queue_cond);

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(th[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&queue_cond);
    free(y);

    return 0;
}
