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
#include "search.h"

#define FILENAME_COLOR "\033[32m"
#define YELLOW_COLOR "\033[33m"
#define RED_COLOR "\033[31m"
#define LINE_NO_COLOR "\033[33m"
#define MATCH_WORD_COLOR "\033[43m\033[30m"
#define RESET_COLOR "\033[49m\033[39m"
#define N 65535

char *pattern = "include";

file_queue *queue;
pthread_mutex_t mutex;
pthread_mutex_t print_mutex;
pthread_cond_t queue_cond;
pthread_cond_t print_cond;
bool hoge = false;

void print_result(const char *buf, const char *filename, match *matches, int mlen)
{
    printf("%s%s%s\n", FILENAME_COLOR, filename, RESET_COLOR);

    int pattern_length = strlen(pattern);
    for (int i = 0; i < mlen; ) {
        int current_line_no = matches[i].line_no;
        int begin = matches[i].start;
        int end   = begin;

        while (begin > 0 && buf[begin - 1] != 0x0A && buf[begin - 1] != 0x0D) {
            begin--;
        }
        while (buf[end] != 0x0A && buf[end] != 0x0D) {
            end++;
        }

        char *tmp = (char *)malloc(sizeof(char) * (end - begin));

        /* printf("%s%d%s:", LINE_NO_COLOR, current_line_no, RESET_COLOR); */

        /* int print_start = begin; */
        /* while (i < mlen && current_line_no == matches[i].line_no) { */
        /*     int match_start = matches[i].start; */
        /*     int prefix_length = match_start - print_start + 1; */
        /*     char *prefix = (char *)malloc(sizeof(char) * prefix_length); */
        /*  */
        /*     strlcpy(prefix, buf + print_start, prefix_length); */
        /*     printf("%s%s%s%s", prefix, MATCH_WORD_COLOR, pattern, RESET_COLOR); */
        /*     free(prefix); */
        /*  */
        /*     print_start = match_start + pattern_length; */
        /*     i++; */
        /* } */
        /*  */
        /* int suffix_length = end - print_start + 1; */
        /* char *suffix = (char *)malloc(sizeof(char) * suffix_length); */
        /* strlcpy(suffix, buf + print_start, suffix_length); */
        /* printf("%s\n", suffix); */
        /* free(suffix); */
    }
    printf("\n");
}

void *print_worker(void *arg)
{
    file_queue_node *current;
    while (1) {
        pthread_mutex_lock(&print_mutex);

        current = dequeue_string_for_print(queue);
        while (current == NULL || !current->searched) {
            if (current == NULL && hoge) {
                return NULL;
            }

            pthread_cond_wait(&print_cond, &print_mutex);

            if (current == NULL) {
                current = dequeue_string_for_print(queue);
            }
        }

        pthread_mutex_unlock(&print_mutex);

        if (current && current->match_lines) {
            matched_line_queue_node *match_line;
            printf("%s%s%s\n", FILENAME_COLOR, current->filename, RESET_COLOR);
            while ((match_line = dequeue_matched_line(current->match_lines)) != NULL) {
                printf("%s\n", match_line->line);
            }
            printf("\n");
            free_matched_line_queue(current->match_lines);
        }
    }

    return NULL;
}

void *search_worker(void *arg)
{
    int id = *((int *)arg);
    char *buf = (char *)malloc(sizeof(char) * N);

    file_queue_node *current;
    log_d("%dth worker: started\n", id);
    while (1) {
        pthread_mutex_lock(&mutex);
        log_d("%dth worker: lock the mutex\n", id);
        current = dequeue_file_for_search(queue);
        log_d("%dth worker: fetch and it is %s\n", id, current == NULL ? "null" : current->filename);
        if (current == NULL) {
            log_d("%dth worker: so the program is waiting and mutex was released\n", id);
            if (hoge) {
                pthread_mutex_unlock(&mutex);
                log_d("%dth workor die\n", id);
                break;
            }
            pthread_cond_wait(&queue_cond, &mutex);
            current = dequeue_file_for_search(queue);
        }
        pthread_mutex_unlock(&mutex);
        log_d("%dth workor: unlock the mutex and current is %s\n", id, current == NULL ? "null" : current->filename);

        if (current != NULL) {
            int fd = open(current->filename, O_RDONLY);
            log_d("%dth worker: %scatch %s%s\n", id, RED_COLOR, current->filename, RESET_COLOR);
            if (!is_binary(fd)) {
                int read_len, total = 0;
                int pattern_len = strlen(pattern);
                matched_line_queue *match_lines = create_matched_line_queue();
                while ((read_len = read(fd, buf, N)) > 0) {
                    match *matches = (match *)malloc(sizeof(match) * 100);
                    int count = search(buf, read_len, pattern, matches);

                    for (int i = 0; i < count; ) {
                        int end = matches[i].start;
                        while (buf[end] != 0x0A && buf[end] != 0x0D) end++;

                        int current_line_no = matches[i].line_no;
                        int line_start = matches[i].line_start;
                        /* int j = i; */
                        /* while (j < count && current_line_no == matches[j].line_no) j++; */
                        /*  */
                        /* int starts_len = j - i; */
                        /* matched_line_queue_node *node = (matched_line_queue_node *)malloc(sizeof(matched_line_queue_node)); */
                        /* node->line   = (char *)malloc(sizeof(char) * line_len); */
                        /* node->starts = (int *)malloc(sizeof(int) * starts_len); */
                        /*  */
                        /* int index = 0; */
                        /* node->line_len = line_len; */
                        /* node->starts_len = starts_len; */
                        /* strlcpy(node->line, buf + line_start, line_len); */
                        /* while (i < count && current_line_no == matches[i].line_no) { */
                        /*     node->starts[index++] = matches[i++].start; */
                        /* } */

                        int line_len = end - line_start + 1;
                        matched_line_queue_node *node = (matched_line_queue_node *)malloc(sizeof(matched_line_queue_node));
                        node->line = (char *)malloc(sizeof(char) * (line_len + (strlen(MATCH_WORD_COLOR) + strlen(RESET_COLOR)) * (line_len / pattern_len)));
                        sprintf(node->line, "%s%d%s:", LINE_NO_COLOR, current_line_no, RESET_COLOR);
                        int print_start = line_start;
                        while (i < count && current_line_no == matches[i].line_no) {
                            int match_start = matches[i].start;
                            int prefix_length = match_start - print_start + 1;
                            char *prefix = (char *)malloc(sizeof(char) * prefix_length);

                            strlcpy(prefix, buf + print_start, prefix_length);
                            sprintf(node->line + strlen(node->line), "%s%s%s%s", prefix, MATCH_WORD_COLOR, pattern, RESET_COLOR);
                            free(prefix);

                            print_start = match_start + pattern_len;
                            i++;
                        }

                        int suffix_length = end - print_start + 1;
                        char *suffix = (char *)malloc(sizeof(char) * suffix_length);
                        strlcpy(suffix, buf + print_start, suffix_length);
                        sprintf(node->line + strlen(node->line), "%s", suffix);
                        free(suffix);

                        enqueue_matched_line(match_lines, node);
                        total++;
                    }

                    if (read_len < N) {
                        break;
                    }
                }

                if (total > 0) {
                    current->match_lines = match_lines;
                } else {
                    free_matched_line_queue(match_lines);
                }

                /* if (n > 0) { */
                /*     match *matches = (match *)malloc(sizeof(match) * 100); */
                /*     int count = search(buf, n, pattern, matches); */
                /*     if (count > 0) { */
                /*         pthread_mutex_lock(&print_mutex); */
                /*         print_result(buf, current->filename, matches, count); */
                /*         pthread_mutex_unlock(&print_mutex); */
                /*     } */
                /*     free(matches); */
                /* } */
                log_d("%dth worker: search complete %s\n", id, current->filename);
            } else {
                log_d("%dth worker: maybe %s was determined as a binary\n", id, current->filename);
            }

            current->searched = true;
            pthread_cond_signal(&print_cond);
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
            pthread_mutex_lock(&mutex);
            enqueue_file(queue, buf);
            log_d("enqueu %s%s%s\n", YELLOW_COLOR, buf, RESET_COLOR);
            pthread_mutex_unlock(&mutex);
            pthread_cond_signal(&queue_cond);
        }
    }
    closedir(dir);
}

int main(int argc, char **argv)
{
    queue = create_file_queue();

    pattern = argv[2];
    generate_bad_character_table(pattern);

    pthread_cond_init(&queue_cond, NULL);
    pthread_cond_init(&print_cond, NULL);
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&print_mutex, NULL);
    const int THREAD_COUNT = atoi(argv[1]);
    int *id = (int *)malloc(THREAD_COUNT * sizeof(int));
    pthread_t th[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; i++) {
        id[i] = i;
        pthread_create(&th[i], NULL, (void *)search_worker, (void *)&id[i]);
    }

    pthread_t pth;
    pthread_create(&pth, NULL, (void *)print_worker, NULL);

    find_target_files2(".");

    hoge = true;
    pthread_cond_broadcast(&queue_cond);

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(th[i], NULL);
    }
    pthread_join(pth, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&print_mutex);
    pthread_cond_destroy(&queue_cond);
    pthread_cond_destroy(&print_cond);

    return 0;
}
