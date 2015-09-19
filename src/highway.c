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
#include "color.h"

static bool complete_file_finding = false;

bool is_complete_file_finding()
{
    return complete_file_finding;
}

bool find_target_files(file_queue *queue, char *dirname)
{
    DIR *dir = opendir(dirname);
    if (dir == NULL) {
        if (access(dirname, F_OK) == 0) {
            pthread_mutex_lock(&file_mutex);
            enqueue_file(queue, dirname);
            pthread_mutex_unlock(&file_mutex);
            pthread_cond_signal(&file_cond);
            return true;
        } else {
            log_e("'%s' can't be opened. Is there the directory or file on your current directory?", dirname);
            return false;
        }
    }

    char buf[1024];
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (is_ignore_directory(entry)) {
            continue;
        }

        sprintf(buf, "%s/%s", dirname, entry->d_name);

        if (is_directory(entry)) {
            find_target_files(queue, buf);
        } else if (is_search_target(entry)) {
            pthread_mutex_lock(&file_mutex);
            enqueue_file(queue, buf);
            pthread_mutex_unlock(&file_mutex);
            pthread_cond_signal(&file_cond);
        }
    }

    closedir(dir);
    return true;
}

int main(int argc, char **argv)
{
    if (!init_mutex()) {
        return 1;
    }

    int return_code = 0;
    hw_option op;
    init_option(argc, argv, &op);

    file_queue *queue = create_file_queue();
    generate_bad_character_table(op.pattern);

    search_worker_params params = { queue, op.pattern };
    pthread_t th[op.worker], pth;
    for (int i = 0; i < op.worker; i++) {
        pthread_create(&th[i], NULL, (void *)search_worker, (void *)&params);
    }
    pthread_create(&pth, NULL, (void *)print_worker, (void *)queue);
    log_d("%d threads was launched for searching.", op.worker);

    for (int i = 0; i < op.patsh_count; i++) {
        if (!find_target_files(queue, op.root_paths[i])) {
            return_code = 1;
            break;
        }
    }
    complete_file_finding = true;
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

    return return_code;
}
