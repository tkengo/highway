#include <stdio.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include "worker.h"
#include "file.h"
#include "log.h"
#include "color.h"

pthread_mutex_t file_mutex;
pthread_mutex_t print_mutex;
pthread_cond_t file_cond;
pthread_cond_t print_cond;

bool init_mutex()
{
    if (pthread_cond_init(&file_cond, NULL)) {
        log_e("A condition variable for file finding was not able to initialize.");
        return false;
    }

    if (pthread_cond_init(&print_cond, NULL)) {
        log_e("A condition variable for print result was not able to initialize.");
        return false;
    }

    if (pthread_mutex_init(&file_mutex, NULL)) {
        log_e("A mutex for file finding was not able to initialize.");
        return false;
    }

    if (pthread_mutex_init(&print_mutex, NULL)) {
        log_e("A mutex for print result was not able to initialize.");
        return false;
    }

    return true;
}

void *print_worker(void *arg)
{
    file_queue *queue = (file_queue *)arg;

    file_queue_node *current;
    while (1) {
        pthread_mutex_lock(&print_mutex);

        current = dequeue_string_for_print(queue);
        while (current == NULL || !current->searched) {
            if (current == NULL && is_complete_finding_file()) {
                pthread_mutex_unlock(&print_mutex);
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
            char *filename = current->filename;
            if (filename[0] == '.' && filename[1] == '/') {
                filename += 2;
            }
            printf("%s%s%s\n", FILENAME_COLOR, filename, RESET_COLOR);
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
    search_worker_params *params = (search_worker_params *)arg;

    file_queue_node *current;
    char *buf = (char *)malloc(sizeof(char) * N);

    while (1) {
        pthread_mutex_lock(&file_mutex);
        current = dequeue_file_for_search(params->queue);
        if (current == NULL) {
            if (is_complete_finding_file()) {
                pthread_mutex_unlock(&file_mutex);
                break;
            }
            pthread_cond_wait(&file_cond, &file_mutex);
            current = dequeue_file_for_search(params->queue);
        }
        pthread_mutex_unlock(&file_mutex);

        if (current != NULL) {
            int fd = open(current->filename, O_RDONLY);
            if (!is_binary(fd)) {
                matched_line_queue *match_lines = create_matched_line_queue();
                int total = search(fd, buf, params->pattern, match_lines);

                if (total > 0) {
                    current->match_lines = match_lines;
                } else {
                    free_matched_line_queue(match_lines);
                }
            }

            current->searched = true;
            pthread_cond_signal(&print_cond);
            close(fd);
        }
    }

    free(buf);

    return NULL;
}
