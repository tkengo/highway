#include <stdio.h>
#include <dirent.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include "highway.h"
#include "option.h"
#include "file.h"
#include "queue.h"
#include "log.h"
#include "search.h"
#include "color.h"

typedef struct _search_worker_params {
    file_queue *queue;
    char *pattern;
} search_worker_params;

bool complete_file_finding = false;
pthread_mutex_t file_mutex;
pthread_mutex_t print_mutex;
pthread_cond_t file_cond;
pthread_cond_t print_cond;

void *print_worker(void *arg)
{
    file_queue *queue = (file_queue *)arg;

    file_queue_node *current;
    while (1) {
        pthread_mutex_lock(&print_mutex);

        current = dequeue_string_for_print(queue);
        while (current == NULL || !current->searched) {
            if (current == NULL && complete_file_finding) {
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
    search_worker_params *params = (search_worker_params *)arg;

    file_queue_node *current;
    char *buf = (char *)malloc(sizeof(char) * N);

    while (1) {
        pthread_mutex_lock(&file_mutex);
        current = dequeue_file_for_search(params->queue);
        if (current == NULL) {
            if (complete_file_finding) {
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

void find_target_files2(file_queue *queue, char *dirname)
{
    char buf[1024];
    struct dirent *entry;
    DIR *dir = opendir(dirname);

    while ((entry = readdir(dir)) != NULL) {
        if (is_ignore_directory(entry)) {
            continue;
        }

        sprintf(buf, "%s/%s", dirname, entry->d_name);

        if (is_directory(entry)) {
            find_target_files2(queue, buf);
        } else if (is_search_target(entry)) {
            pthread_mutex_lock(&file_mutex);
            enqueue_file(queue, buf);
            pthread_mutex_unlock(&file_mutex);
            pthread_cond_signal(&file_cond);
        }
    }
    closedir(dir);
}

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

int main(int argc, char **argv)
{
    if (!init_mutex()) {
        return -1;
    }

    init_option(argc, argv);

    file_queue *queue = create_file_queue();
    char *pattern = argv[2];
    generate_bad_character_table(pattern);

    search_worker_params params = { queue, pattern };
    const int THREAD_COUNT = atoi(argv[1]);
    pthread_t th[THREAD_COUNT], pth;
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_create(&th[i], NULL, (void *)search_worker, (void *)&params);
    }
    pthread_create(&pth, NULL, (void *)print_worker, (void *)queue);

    find_target_files2(queue, ".");

    complete_file_finding = true;
    pthread_cond_broadcast(&file_cond);

    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(th[i], NULL);
    }
    pthread_join(pth, NULL);

    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&print_mutex);
    pthread_cond_destroy(&file_cond);
    pthread_cond_destroy(&print_cond);

    return 0;
}
