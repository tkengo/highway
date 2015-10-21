#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "highway.h"
#include "hwmalloc.h"
#include "worker.h"
#include "scan.h"
#include "option.h"
#include "fjs.h"
#include "log.h"
#include "print.h"
#include "worker.h"
#include "util.h"
#include "regex.h"

static bool complete_scan_file = false;

bool is_complete_scan_file()
{
    return complete_scan_file;
}

int process_terminal()
{
    int error_no;
    file_queue *queue = create_file_queue();

    // Launch some worker threads for searching.
    pthread_t th[op.worker], pth;
    worker_params search_params[op.worker];
    int launched_worker_count = 0;
    for (int i = 0; i < op.worker; i++) {
        search_params[i].index = i;
        search_params[i].queue = queue;
        if ((error_no = pthread_create(&th[i], NULL, (void *)search_worker, (void *)&search_params[i])) == 0) {
            launched_worker_count++;
        }
    }
    if (launched_worker_count == 0) {
        tc_free(queue);
        log_e("Failed launch search worker: error no = %d", error_no);
        return -1;
    }

    // Launch one threads for printing result.
    worker_params print_params = { op.worker, queue };
    if ((error_no = pthread_create(&pth, NULL, (void *)print_worker, (void *)&print_params)) != 0) {
        tc_free(queue);
        log_e("Failed launch print worker: error no = %d", error_no);
        return -1;
    }
    log_d("Worker num: %d", op.worker);

    // Scan target files recursively. If target files was found, they are added to the file queue,
    // and then search worker will take items from the queue and do searching.
    for (int i = 0; i < op.paths_count; i++) {
        scan_target(queue, op.root_paths[i], NULL, 0);
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
    enum file_type t = FILE_TYPE_UTF8;

    if (!op.use_regex) {
        prepare_fjs(pattern, pattern_len, t);
    }

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

    if (op.use_regex && !onig_init_wrap()) {
        return 1;
    }

    int return_code = 0;
    if (op.stdin_redirect) {
        setvbuf(stdout, NULL, _IONBF, 0);
        return_code = process_stdin();
    } else {
        set_fd_rlimit(MAX_FD_NUM);
        setvbuf(stdout, NULL, _IOFBF, 1024 * 64);
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
