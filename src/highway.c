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
#include "worker.h"
#include "ignore.h"
#include "util.h"
#include "color.h"
#include "oniguruma.h"

static bool complete_finding_file = false;

bool is_complete_finding_file()
{
    return complete_finding_file;
}

void enqueue_file_exclusively(file_queue *queue, const char *filename)
{
    pthread_mutex_lock(&file_mutex);
    enqueue_file(queue, filename);
    pthread_mutex_unlock(&file_mutex);
    pthread_cond_signal(&file_cond);
}

/**
 * Find search target files recursively under the specified directories.
 *
 * @param queue A target file is added to this queue.
 * @param dirname A directory name or filename.
 * @param ignores Ignore list.
 */
bool find_target_files(file_queue *queue, const char *base, const char *dirname, ignore_list *ignores)
{
    char buf[1024];

    // Open the dirname as a directory. If it is not a directory, check whether if it is a file,
    // and then add the file to the queue if it is true.
    DIR *dir = opendir(dirname);
    if (dir == NULL) {
        if (access(dirname, F_OK) == 0) {
            enqueue_file_exclusively(queue, dirname);
            return true;
        } else {
            log_e("'%s' can't be opened. Is there the directory or file on your current directory?", dirname);
            return false;
        }
    }

    if (ignores == NULL) {
        sprintf(buf, "%s/%s", dirname, ".gitignore");
        ignores = create_ignore_list_from_gitignore(buf);
    }

    int offset = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // `readdir` returns also current or upper directory, but we don't need that directories,
        // so skip them. And also .git directory doesn't need.
        if (is_ignore_directory(entry)) {
            continue;
        }

        sprintf(buf, "%s/%s", dirname, entry->d_name);
        if (ignores != NULL && is_ignore(ignores, base, buf, entry)) {
            continue;
        }

        if (is_directory(entry)) {
            find_target_files(queue, base, buf + offset, ignores);
        } else if (is_search_target(entry)) {
            enqueue_file_exclusively(queue, buf + offset);
        }
    }

    closedir(dir);
    return true;
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

    if (op.use_regex) {
        onig_init();
    }

    file_queue *queue = create_file_queue();
    worker_params params = { queue, &op };
    pthread_t th[op.worker], pth;
    for (int i = 0; i < op.worker; i++) {
        pthread_create(&th[i], NULL, (void *)search_worker, (void *)&params);
    }
    pthread_create(&pth, NULL, (void *)print_worker, (void *)&params);
    log_d("%d threads was launched for searching.", op.worker);

    for (int i = 0; i < op.paths_count; i++) {
        if (!find_target_files(queue, op.root_paths[i], op.root_paths[i], NULL)) {
            return_code = 1;
            break;
        }
    }
    complete_finding_file = true;
    pthread_cond_broadcast(&file_cond);
    pthread_cond_broadcast(&print_cond);

    for (int i = 0; i < op.worker; i++) {
        pthread_join(th[i], NULL);
    }
    pthread_join(pth, NULL);

    pthread_mutex_destroy(&file_mutex);
    pthread_mutex_destroy(&print_mutex);
    pthread_cond_destroy(&file_cond);
    pthread_cond_destroy(&print_cond);
    free_file_queue(queue);
    close_iconv();

    if (op.use_regex) {
        onig_end();
    }

    return return_code;
}
