#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <gperftools/tcmalloc.h>
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
bool find_target_files(file_queue *queue, const char *dir_path, ignore_hash *ignores, int depth)
{
    char buf[MAX_PATH_LENGTH], base[MAX_PATH_LENGTH];

    // Open the path as a directory. If it is not a directory, check whether if it is a file,
    // and then add the file to the queue if it is true.
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        if (access(dir_path, F_OK) == 0) {
            enqueue_file_exclusively(queue, dir_path);
            return true;
        } else {
            log_e("'%s' can't be opened. Is there the directory or file on your current directory?", dir_path);
            return false;
        }
    }

    if (strcmp(dir_path, ".") == 0) {
        strcpy(base, "");
    } else {
        sprintf(base, "%s/", dir_path);
    }

    bool need_free = false;
    if (!op.all_files) {
        sprintf(buf, "%s%s", base, ".gitignore");
        if (access(buf, F_OK) == 0) {
            // Create search ignore list from the .gitignore file. New list is created if there are
            // not ignore file upper directories, otherwise the list will be inherited.
            if (ignores == NULL) {
                ignores = load_ignore_hash(base, buf, depth);
                need_free = true;
            } else {
                ignores = merge_ignore_hash(ignores, base, buf, depth);
            }
        }
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // `readdir` returns also current or upper directory, but we don't need that directories,
        // so skip them. And also hidden directory doesn't need.
        if (is_skip_entry(entry)) {
            continue;
        }

        sprintf(buf, "%s%s", base, entry->d_name);

        // Check whether if the file is ignored by gitignore. If it is ignored, skip finding.
        if (!op.all_files && ignores != NULL && is_ignore(ignores, buf, entry, depth)) {
            continue;
        }

        // Check if symlink exists. Skip this entry if not exist.
        if (op.follow_link && entry->d_type == DT_LNK) {
            char link[MAX_PATH_LENGTH] = { 0 };
            readlink(buf, link, MAX_PATH_LENGTH);
            if (access(link, F_OK) != 0) {
                continue;
            }
        }

        if (is_directory(entry)) {
            find_target_files(queue, buf, ignores, depth + 1);
        } else if (is_search_target(entry)) {
            enqueue_file_exclusively(queue, buf);
        }
    }

    if (!op.all_files && ignores != NULL) {
        free_ignore_hash(ignores, depth);

        if (need_free) {
            tc_free(ignores);
        }
    }

    closedir(dir);
    return true;
}

int process_by_terminal()
{
    file_queue *queue = create_file_queue();

    // Launch worker count threads for searching pattern from files.
    pthread_t th[op.worker], pth;
    worker_params search_params[op.worker];
    for (int i = 0; i < op.worker; i++) {
        search_params[i].index = i;
        search_params[i].queue = queue;
        pthread_create(&th[i], NULL, (void *)search_worker, (void *)&search_params[i]);
    }

    // Launch one threads for printing result.
    worker_params print_params = { op.worker, queue };
    pthread_create(&pth, NULL, (void *)print_worker, (void *)&print_params);
    log_d("Worker num: %d", op.worker);

    for (int i = 0; i < op.paths_count; i++) {
        find_target_files(queue, op.root_paths[i], NULL, 0);
    }
    complete_finding_file = true;
    pthread_cond_broadcast(&file_cond);
    pthread_cond_broadcast(&print_cond);

    for (int i = 0; i < op.worker; i++) {
        pthread_join(th[i], NULL);
    }
    pthread_join(pth, NULL);
    free_file_queue(queue);

    return 0;
}

int process_by_redirection()
{
    matched_line_queue *match_line = create_matched_line_queue();
    int match_count = search(STDIN_FILENO, op.pattern, strlen(op.pattern), FILE_TYPE_UTF8, match_line, 0);

    if (match_count > 0) {
        char *filename = "stream";
        file_queue_node dummy;
        dummy.t           = FILE_TYPE_UTF8;
        dummy.match_lines = match_line;

        if (op.stdout_redirect) {
            print_redirection(filename, &dummy);
        } else {
            print_to_terminal(filename, &dummy);
        }
    }

    return 0;
}

/**
 * Entry point.
 */
int main(int argc, char **argv)
{
    int return_code = 0;

    if (!init_mutex()) {
        return 1;
    }

    init_option(argc, argv, &op);
    init_iconv();

    if (op.use_regex && !onig_init_wrap()) {
        return 1;
    }

    setvbuf(stdout, NULL, _IOFBF, 1024 * 64);

    if (op.stdin_redirect) {
        return_code = process_by_redirection();
    } else {
        return_code = process_by_terminal();
    }

    destroy_mutex();
    close_iconv();
    free_option(&op);

    if (op.use_regex) {
        onig_end_wrap();
    } else {
        free_fjs();
    }

    log_flush();
    return return_code;
}
