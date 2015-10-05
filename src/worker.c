#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <iconv.h>
#include <unistd.h>
#include "worker.h"
#include "file.h"
#include "log.h"
#include "util.h"
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

void destroy_mutex()
{
    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&print_mutex);
    pthread_cond_destroy(&file_cond);
    pthread_cond_destroy(&print_cond);
}

void print_to_terminal(const char *filename, file_queue_node *current, const hw_option *op)
{
    matched_line_queue_node *match_line;
    printf("%s%s%s\n", FILENAME_COLOR, filename, RESET_COLOR);

    // If `file_with_matches` option is available, match results don't print on console.
    if (!op->file_with_matches) {
        while ((match_line = dequeue_matched_line(current->match_lines)) != NULL) {
            // Print colorized line number.
            printf("%s%d%s:", LINE_NO_COLOR, match_line->line_no, RESET_COLOR);

            if (current->t == FILE_TYPE_UTF8) {
                printf("%s", match_line->line);
            } else {
                const int line_len = strlen(match_line->line), utf8_len_guess = line_len * 2;
                char out[utf8_len_guess];
                memset(out, 0, utf8_len_guess);
                if (current->t == FILE_TYPE_EUC_JP) {
                    to_utf8_from_euc(match_line->line, line_len, out, utf8_len_guess);
                } else if (current->t == FILE_TYPE_SHIFT_JIS) {
                    to_utf8_from_sjis(match_line->line, line_len, out, utf8_len_guess);
                }
                printf("%s", out);
            }
            printf("\n");
        }
        printf("\n");
    }
}

void print_redirection(const char *filename, file_queue_node *current, const hw_option *op)
{
    matched_line_queue_node *match_line;

    if (op->file_with_matches) {
        // If `file_with_matches` option is available, we print only filenames.
        printf("%s\n", filename);
    } else {
        while ((match_line = dequeue_matched_line(current->match_lines)) != NULL) {
            printf("%s:%d:", filename, match_line->line_no);

            if (current->t == FILE_TYPE_UTF8) {
                printf("%s", match_line->line);
            } else {
                const int line_len = strlen(match_line->line), utf8_len_guess = line_len * 2;
                char out[utf8_len_guess];
                memset(out, 0, utf8_len_guess);
                if (current->t == FILE_TYPE_EUC_JP) {
                    to_utf8_from_euc(match_line->line, line_len, out, utf8_len_guess);
                } else if (current->t == FILE_TYPE_SHIFT_JIS) {
                    to_utf8_from_sjis(match_line->line, line_len, out, utf8_len_guess);
                }
                printf("%s", out);
            }
            printf("\n");
        }
    }
}

/**
 * Print worker. This method was invoked from search worker when searching was completed.
 */
void *print_worker(void *arg)
{
    worker_params *params = (worker_params *)arg;
    file_queue *queue = params->queue;

    while (1) {
        // This worker takes out a print target file from the queue. If the queue is empty, worker
        // will be waiting until find at least one target print file.
        pthread_mutex_lock(&print_mutex);
        file_queue_node *current = peek_file_for_print(queue);
        while (current == NULL || !current->searched) {
            // Break this loop if all files was searched.
            if (current == NULL && is_complete_finding_file()) {
                pthread_mutex_unlock(&print_mutex);
                return NULL;
            }

            pthread_cond_wait(&print_cond, &print_mutex);

            if (current == NULL) {
                current = peek_file_for_print(queue);
            }
        }
        pthread_mutex_unlock(&print_mutex);

        if (current && current->matched) {
            // Remove ./ from the filename in order to make easy to see.
            char *filename = current->filename;
            if (filename[0] == '.' && filename[1] == '/') {
                filename += 2;
            }

            if (IS_STDOUT_REDIRECT) {
                print_redirection(filename, current, params->op);
            } else {
                print_to_terminal(filename, current, params->op);
            }

            if (current->omit_matches) {
                log_buffered("%s%s%s: Some results was omitted because too many matches was found, so you might want to specify --no-omit option", FILENAME_COLOR, current->filename, RESET_COLOR);
            }

            free_matched_line_queue(current->match_lines);
        }
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
    const size_t out_len = 1024;

    while (1) {
        // This worker takes out a search target file from the queue. If the queue is empty, worker
        // will be waiting until find at least one target file.
        pthread_mutex_lock(&file_mutex);
        file_queue_node *current = peek_file_for_search(params->queue);
        if (current == NULL) {
            // Break this loop if all files was searched.
            if (is_complete_finding_file()) {
                pthread_mutex_unlock(&file_mutex);
                break;
            }
            pthread_cond_wait(&file_cond, &file_mutex);
            current = peek_file_for_search(params->queue);
        }
        pthread_mutex_unlock(&file_mutex);

        if (current == NULL) {
            continue;
        }

        // Check file type of the target file, then if it is a binary, we skip it because the hw is
        // the software in order to search "source code", so searching binary files is not good.
        int fd = open(current->filename, O_RDONLY);
        if (fd != -1 && (t = detect_type_type(fd)) != FILE_TYPE_BINARY) {
            char *pattern = params->op->pattern;

            // Convert the pattern to appropriate encoding if an encoding of the target file is
            // not UTF-8.
            char out[out_len + 1];
            if (t == FILE_TYPE_EUC_JP || t == FILE_TYPE_SHIFT_JIS) {
                if (t == FILE_TYPE_EUC_JP) {
                    to_euc(params->op->pattern, params->op->pattern_len, out, out_len);
                } else if (t == FILE_TYPE_SHIFT_JIS) {
                    to_sjis(params->op->pattern, params->op->pattern_len, out, out_len);
                }
                pattern = out;
            }

            // Searching.
            int actual_match_count = 0;
            matched_line_queue *match_line = create_matched_line_queue();
            int match_count = search(fd, pattern, t, params->op, match_line);

            if (match_count > 0) {
                // Set additional data to the queue data because it will be used on print worker in
                // order to print results to the console. `match_line` variable will be released
                // along with the file queue when it is released.
                current->matched      = true;
                current->match_lines  = match_line;
                current->t            = t;
                current->omit_matches = match_count < actual_match_count;
            } else {
                // If the pattern was not matched, the lines queue is no longer needed, so do free.
                free_matched_line_queue(match_line);
            }
        }

        // Launch print worker.
        current->searched = true;
        pthread_cond_signal(&print_cond);

        close(fd);
    }

    return NULL;
}
