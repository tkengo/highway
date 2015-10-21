#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <iconv.h>
#include <unistd.h>
#include <math.h>
#include "worker.h"
#include "hwmalloc.h"
#include "print.h"
#include "file.h"
#include "log.h"
#include "util.h"
#include "color.h"
#include "line_list.h"

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

void destroy_mutex()
{
    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&print_mutex);
    pthread_cond_destroy(&file_cond);
    pthread_cond_destroy(&print_cond);
}

/**
 * Print worker. This method was invoked from search worker when searching was completed.
 */
void *print_worker(void *arg)
{
    worker_params *params = (worker_params *)arg;
    file_queue *queue = params->queue;

    // We should wait for queuing first queue item because print worker is launched before scanning
    // target file.
    pthread_mutex_lock(&print_mutex);
    while (queue->first == NULL) {
        if (is_complete_scan_file()) {
            break;
        }
        pthread_cond_wait(&print_cond, &print_mutex);
    }
    pthread_mutex_unlock(&print_mutex);

    // If the file queue is empty, it will do nothing.
    if (queue->first == NULL) {
        return NULL;
    }

    file_queue_node *current = queue->first;
    file_queue_node *prev = current->prev;
    while (1) {
        // This worker takes out a print target file from the queue. If the queue is empty, worker
        // will be waiting until find at least one target print file.
        pthread_mutex_lock(&print_mutex);
        while (current == NULL || !current->searched) {
            // Break this loop if all files was searched.
            if (current == NULL && is_complete_scan_file()) {
                pthread_mutex_unlock(&print_mutex);
                return NULL;
            }
            pthread_cond_wait(&print_cond, &print_mutex);
            if (current == NULL) {
                current = prev->next;
            }
        }
        pthread_mutex_unlock(&print_mutex);

        if (current == NULL) {
            continue;
        }

        if (current->matched) {
            print_result(current);

            // Insert new line to separate from previouse group if --group options is available.
            if (!op.file_with_matches && op.group) {
                fputs("\n", stdout);
            }

            free_match_line_list(current->match_lines);
        }

        // Current node is used on scaning target, so it should not release it's memory now.
        // Therefore release memory of the previous node because it is no longer used in the
        // future.
        if (current->prev) {
            tc_free(current->prev);
        }

        prev = current;
        current = current->next;
    }

    return NULL;
}

/**
 * Search worker. This method was invoked on threads launched from `main`.
 */
void *search_worker(void *arg)
{
    enum file_type t;
    worker_params *params = (worker_params *)arg;
    file_queue_node *current;
    const size_t out_len = 1024;
    int utf8_pattern_len = strlen(op.pattern);

    while (1) {
        // This worker takes out a search target file from the queue. If the queue is empty, worker
        // will be waiting until find at least one target file.
        pthread_mutex_lock(&file_mutex);
        while ((current = peek_file_for_search(params->queue)) == NULL) {
            // Break this loop if all files was searched.
            if (is_complete_scan_file()) {
                pthread_mutex_unlock(&file_mutex);
                return NULL;
            }
            pthread_cond_wait(&file_cond, &file_mutex);
        }
        pthread_mutex_unlock(&file_mutex);

        // Check file type of the target file, then if it is a binary, we skip it because the hw is
        // the software in order to search "source code", so searching binary files is not good.
        int fd = open(current->filename, O_RDONLY);
        if (fd != -1 && (t = detect_file_type(fd)) != FILE_TYPE_BINARY) {
            char *pattern = op.pattern;

            // Convert the pattern to appropriate encoding if an encoding of the target file is
            // not UTF-8.
            char out[out_len + 1], pattern_len = utf8_pattern_len;
            if (t == FILE_TYPE_EUC_JP || t == FILE_TYPE_SHIFT_JIS) {
                if (t == FILE_TYPE_EUC_JP) {
                    to_euc(op.pattern, utf8_pattern_len, out, out_len);
                } else if (t == FILE_TYPE_SHIFT_JIS) {
                    to_sjis(op.pattern, utf8_pattern_len, out, out_len);
                }
                pattern = out;
                pattern_len = strlen(pattern);
            }

            // Searching.
            match_line_list *match_lines = create_match_line_list();
            int match_count = search(fd, pattern, pattern_len, t, match_lines, params->index);

            if (match_count > 0) {
                // Set additional data to the queue data because it will be used on print worker in
                // order to print results to the console. `match_lines` variable will be released
                // along with the file queue when it is released.
                current->matched      = true;
                current->match_lines  = match_lines;
                current->t            = t;
            } else {
                // If the pattern was not matched, the lines queue is no longer needed, so do free.
                free_match_line_list(match_lines);
            }
        }

        // Wake up print worker.
        current->searched = true;
        pthread_cond_signal(&print_cond);

        close(fd);
    }

    return NULL;
}
