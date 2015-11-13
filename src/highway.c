#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <locale.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "highway.h"
#include "hwmalloc.h"
#include "scan.h"
#include "worker.h"
#include "scan.h"
#include "option.h"
#include "fjs.h"
#include "log.h"
#include "print.h"
#include "worker.h"
#include "util.h"
#include "regex.h"
#include "file.h"

static bool complete_scan_file = false;

bool is_complete_scan_file()
{
    return complete_scan_file;
}

int process_terminal()
{
    int r;
    file_queue *queue = create_file_queue();

    // Launch some worker threads for searching.
    pthread_t th[op.worker], pth;
    worker_params search_params[op.worker];
    for (int i = 0; i < op.worker; i++) {
        search_params[i].index = i;
        search_params[i].queue = queue;
        if ((r = pthread_create(&th[i], NULL, (void *)search_worker, (void *)&search_params[i])) != 0) {
            tc_free(queue);
            log_e("Error in search pthread_create. %s (%d)", strerror(r), r);
            return 1;
        }
    }

    // Launch one thread for printing result.
    worker_params print_params = { op.worker, queue };
    if ((r = pthread_create(&pth, NULL, (void *)print_worker, (void *)&print_params)) != 0) {
        tc_free(queue);
        log_e("Error in print pthread_create. %s (%d)", strerror(r), r);
        return 1;
    }
    log_d("Worker num: %d", op.worker);

    ignore_hash *ignores = NULL;
    if (!op.has_dot_path) {
        ignores = load_ignore_hash("", GIT_IGNORE_NAME, 0);
    }

    // Scan target files recursively. If target files was found, they are added to the file queue,
    // and then search worker will take items from the queue and do searching.
    struct stat st;
    for (int i = 0; i < op.paths_count; i++) {
        char *path = op.root_paths[i];
        if (stat(path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                scan_target(queue, path, strcmp(path, ".") == 0 ? NULL : ignores, 0);
            } else if (is_search_target_by_stat(&st)) {
                enqueue_file_exclusively(queue, path);
            }
        } else {
            log_w("'%s' can't be opened. %s (%d)", path, strerror(errno), errno);
        }
    }
    complete_scan_file = true;
    pthread_cond_broadcast(&file_cond);

    // Wait for completion of searching threads.
    for (int i = 0; i < op.worker; i++) {
        pthread_join(th[i], NULL);
    }

    pthread_mutex_lock(&print_mutex);
    pthread_cond_signal(&print_cond);
    pthread_mutex_unlock(&print_mutex);
    pthread_join(pth, NULL);

    if (queue->last) {
        tc_free(queue->last);
    }
    tc_free(queue);

    return 0;
}

int process_stdin()
{
    char *pattern = op.pattern;
    int pattern_len = strlen(op.pattern);
    enum file_type t = locale_enc();

    match_line_list *match_lines = create_match_line_list();
    search(STDIN_FILENO, pattern, pattern_len, t, match_lines, 0);
    free_match_line_list(match_lines);

    return 0;
}

/**
 * Entry point.
 */
int main(int argc, char **argv)
{
    init_option(argc, argv);

    if (!init_mutex()) {
        return 1;
    }

    init_iconv();
    setlocale(LC_ALL, "");

    if (op.use_regex && !onig_init_wrap()) {
        return 1;
    }

    int return_code = 0;

    if (op.stdin_redirect) {
        setvbuf(stdout, NULL, _IONBF, 0);
        return_code = process_stdin();
    } else {
        if (op.buffering) {
            setvbuf(stdout, NULL, _IOFBF, 1024 * 64);
        }
        set_fd_rlimit(MAX_FD_NUM);
        return_code = process_terminal();
    }

    destroy_mutex();
    close_iconv();
    free_option();

    if (op.use_regex) {
        onig_end_wrap();
    } else {
        free_fjs();
    }

    log_flush();
    return return_code;
}
