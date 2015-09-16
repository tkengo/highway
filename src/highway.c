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
#include "log.h"

#define TABLE_SIZE 256
#define FILENAME_COLOR "\033[1;32m"
#define TMP_COLOR "\033[1;33m"
#define RESET_COLOR "\033[0m\033[K"
#define N 65535

char tbl[TABLE_SIZE];
char *pattern = "include";

typedef struct _match {
    int start;
    int line_no;
} match;

int search(const unsigned char *y, int n, const char *x, match *matches)
{
    int i, j = 0, m = strlen(x);

    int count = 0;
    int line_no = 1;
    char firstCh = x[0];
    char lastCh  = x[m - 1];
    while (j <= n - m) {
        if (lastCh == y[j + m - 1] && firstCh == y[j]) {
            for (i = m - 2; i >= 0 && x[i] == y[j + i]; --i) {
                if (i <= 0) {
                    // matched
                    matches[count].start = j;
                    matches[count].line_no = line_no;
                    count++;
                }
            }
        }
        int skip = tbl[y[j + m]];
        for (int k = 0; k < skip; k++) {
            int t = y[j + k];
            if (t == 0x0A || t == 0x0D) {
                line_no++;
            }
        }
        j += skip;
    }

    return count;
}

file_queue *queue;
pthread_mutex_t mutex;
pthread_cond_t queue_cond;
bool hoge = false;

void print_result(const char *buf, const char *filename, match *matches, int mlen)
{
    printf("%s%s%s\n", FILENAME_COLOR, filename, RESET_COLOR);

    for (int i = 0; i < mlen; i++) {
        int start = matches[i].start;

        int begin = start;
        int end   = start;
        while (begin >= 0 && buf[begin] != 0x0A && buf[begin] != 0x0D) {
            begin--;
        }
        while (buf[end] != 0x0A && buf[end] != 0x0D) {
            end++;
        }

        begin++;
        int llen = end - begin;
        char *line = (char *)malloc(sizeof(char) * llen);
        strncpy(line, buf + begin, llen);
        printf("%d:%s\n", matches[i].line_no, line);
    }
    printf("\n");
}

void *worker(void *arg)
{
    int id = *((int *)arg);
    unsigned char *buf = (unsigned char *)malloc(sizeof(unsigned char) * N);

    file_queue_node *current;
    log_d("%dth worker: started\n", id);
    while (1) {
        pthread_mutex_lock(&mutex);
        log_d("%dth worker: lock the mutex\n", id);
        current = dequeue_file(queue);
        log_d("%dth worker: fetch and it is %s\n", id, current == NULL ? "null" : current->filename);
        if (current == NULL) {
            log_d("%dth worker: but null, mutex was released\n", id);
            if (hoge) {
                pthread_mutex_unlock(&mutex);
                log_d("%dth workor die\n", id);
                break;
            }
            pthread_cond_wait(&queue_cond, &mutex);
            current = dequeue_file(queue);
        }
        pthread_mutex_unlock(&mutex);

        if (current != NULL) {
            int fd = open(current->filename, O_RDONLY);
            log_d("%dth worker: %s\n", id, current->filename);
            if (!is_binary(fd)) {
                int n = read(fd, buf, N);
                if (n > 0) {
                    match *matches = (match *)malloc(sizeof(match) * 100);
                    int count = search(buf, n, pattern, matches);
                    if (count > 0) {
                        print_result((char *)buf, current->filename, matches, count);
                    }
                    free(matches);
                }
            }

            close(fd);
        }
    }

    free(buf);

    return NULL;
}

void find_target_files2(char *dirname)
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
            find_target_files2(buf);
        } else if (is_search_target(entry)) {
            log_d("%s%s%s\n", TMP_COLOR, buf, RESET_COLOR);
        pthread_mutex_lock(&mutex);
            enqueue_file(queue, buf);
        pthread_mutex_unlock(&mutex);
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
    const int THREAD_COUNT = atoi(argv[1]);
    int *id = (int *)malloc(THREAD_COUNT * sizeof(int));
    pthread_t th[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        id[i] = i;
        pthread_create(&th[i], NULL, (void *)worker, (void *)&id[i]);
    }

    find_target_files2(".");

    hoge = true;
    pthread_cond_broadcast(&queue_cond);

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(th[i], NULL);
    }
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&queue_cond);

    return 0;
}
