#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "config.h"
#include "scan.h"
#include "hwmalloc.h"
#include "worker.h"
#include "log.h"
#include "util.h"

/**
 * Add a filename to the queue and launch the sleeping search worker.
 */
void enqueue_file_exclusively(file_queue *queue, const char *filename)
{
    pthread_mutex_lock(&file_mutex);
    enqueue_file(queue, filename);
    pthread_cond_signal(&file_cond);
    pthread_mutex_unlock(&file_mutex);
}

/**
 * Check if the directory entry is ignored by the highway. The directory is ignored if it is the
 * current directory or upper directory or hidden directory(started directory name with dot `.`).
 */
bool is_skip_entry(const struct dirent *entry)
{
#ifdef HAVE_STRUCT_DIRENT_D_NAMLEN
    size_t len = entry->d_namlen;
#else
    size_t len = strlen(entry->d_name);
#endif

    bool cur    = len == 1 && entry->d_name[0] == '.';
    bool up     = len == 2 && entry->d_name[0] == '.' && entry->d_name[1] == '.';
    bool hidden = len  > 1 && entry->d_name[0] == '.' && !op.all_files;

    return (ENTRY_ISDIR(entry) && (cur || up)) || hidden;
}

bool is_search_target_by_stat(const struct stat *st)
{
#ifndef _WIN32
    if (op.follow_link) {
        return S_ISREG(st->st_mode) || S_ISLNK(st->st_mode);
    } else {
        return S_ISREG(st->st_mode);
    }
#else
    return 1;
#endif
}

bool is_search_target_by_entry(const struct dirent *entry)
{
#ifndef _WIN32
    if (op.follow_link) {
        return entry->d_type == DT_REG || entry->d_type == DT_LNK;
    } else {
        return entry->d_type == DT_REG;
    }
#else
    return 1;
#endif
}

/**
 * Find search target files recursively under the specified directories, and add filenames to the
 * queue used by search worker.
 */
void scan_target(file_queue *queue, const char *dir_path, ignore_hash *ignores, int depth)
{
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        if (errno == EMFILE) {
            log_w("Too many open files (%s). Try to increase fd limit by `ulimit -n N`.", dir_path);
        } else {
            log_w("%s can't be opend. %s (%d).", dir_path, strerror(errno), errno);
        }
        return;
    }

    char buf[MAX_PATH_LENGTH], base[MAX_PATH_LENGTH];
    if (strcmp(dir_path, ".") == 0) {
        strcpy(base, "");
    } else {
        sprintf(base, "%s/", dir_path);
    }

    bool need_free = false;
    if (!op.all_files) {
        sprintf(buf, "%s%s", base, GIT_IGNORE_NAME);
        if (access(buf, F_OK) == 0) {
            // Create search ignore list from the .gitignore file. New list is created if there are
            // not ignore file on upper directories, otherwise the list will be inherited.
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
#ifndef _WIN32
        if (op.follow_link && entry->d_type == DT_LNK) {
            char link[MAX_PATH_LENGTH] = { 0 };
            readlink(buf, link, MAX_PATH_LENGTH);
            if (access(link, F_OK) != 0) {
                continue;
            }
        }
#endif

        if (ENTRY_ISDIR(entry)) {
            scan_target(queue, buf, ignores, depth + 1);
        } else if (is_search_target_by_entry(entry)) {
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
}
