#include <stdio.h>
#include <string.h>
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
#include "worker.h"
#include "ignore.h"
#include "util.h"
#include "regex.h"
#include "color.h"
#include "oniguruma.h"

static bool complete_finding_file = false;

bool is_complete_finding_file()
{
    return complete_finding_file;
}

/**
 * Add a filename to the queue and launch the sleeping search worker.
 */
void enqueue_file_exclusively(file_queue *queue, const char *filename)
{
    pthread_mutex_lock(&file_mutex);
    enqueue_file(queue, filename);
    pthread_mutex_unlock(&file_mutex);
    pthread_cond_signal(&file_cond);
}

/**
 * Find search target files recursively under the specified directories, and add filenames to the
 * queue used by search worker.
 */
bool find_target_files(file_queue *queue,
                       const char *path,
                       ignore_list *ignores,
                       const hw_option *op)
{
    char buf[1024];

    // Open the path as a directory. If it is not a directory, check whether if it is a file,
    // and then add the file to the queue if it is true.
    DIR *dir = opendir(path);
    if (dir == NULL) {
        if (access(path, F_OK) == 0) {
            enqueue_file_exclusively(queue, path);
            return true;
        } else {
            log_e("'%s' can't be opened. Is there the directory or file on your current directory?", path);
            return false;
        }
    }

    if (!op->all_files) {
        if (ignores == NULL) {
            ignores = create_ignore_list_from_gitignore(path);
        } else {
            ignores = create_ignore_list_from_list(path, ignores);
        }
    }

    int offset = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // `readdir` returns also current or upper directory, but we don't need that directories,
        // so skip them. And also hidden directory doesn't need.
        if (is_skip_directory(entry)) {
            continue;
        }

        sprintf(buf, "%s/%s", path, entry->d_name);
        if (!op->all_files && ignores != NULL && is_ignore(ignores, buf, entry)) {
            continue;
        }

        if (is_directory(entry)) {
            find_target_files(queue, buf + offset, ignores, op);
        } else if (is_search_target(entry)) {
            enqueue_file_exclusively(queue, buf + offset);
        }
    }

    if (ignores != NULL) {
        free_ignore_list(ignores);
    }

    closedir(dir);
    return true;
}

int process_by_terminal(hw_option *op)
{
    file_queue *queue = create_file_queue();
    worker_params params = { queue, op };
    pthread_t th[op->worker], pth;
    for (int i = 0; i < op->worker; i++) {
        pthread_create(&th[i], NULL, (void *)search_worker, (void *)&params);
    }
    pthread_create(&pth, NULL, (void *)print_worker, (void *)&params);
    log_d("%d threads was launched for searching.", op->worker);

    for (int i = 0; i < op->paths_count; i++) {
        find_target_files(queue, op->root_paths[i], NULL, op);
    }
    complete_finding_file = true;
    pthread_cond_broadcast(&file_cond);
    pthread_cond_broadcast(&print_cond);

    for (int i = 0; i < op->worker; i++) {
        pthread_join(th[i], NULL);
    }
    pthread_join(pth, NULL);
    free_file_queue(queue);

    return 0;
}

int process_by_redirection(hw_option *op)
{
    file_queue *queue = create_file_queue();
    enqueue_file(queue, "dummy");
    complete_finding_file = true;

    // Searching.
    int match_line_count = search_stream(op->pattern, op);

    return 0;
}

/**
 * Entry point.
 */
int main(int argc, char **argv)
{
    int return_code = 0;
    hw_option op;

    if (!init_mutex()) {
        return 1;
    }

    init_option(argc, argv, &op);
    init_iconv();

    if (op.use_regex && !onig_init_wrap()) {
        return 1;
    }

    if (stdin_redirect_from()) {
        return_code = process_by_redirection(&op);
    } else {
        return_code = process_by_terminal(&op);
    }

    destroy_mutex();
    close_iconv();

    if (op.use_regex) {
        onig_end_wrap();
    }

    log_flush();
    return return_code;
}
