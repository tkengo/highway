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

    return (entry->d_type == DT_DIR && (cur || up)) || hidden;
}

bool is_search_target(const struct dirent *entry)
{
    if (op.follow_link) {
        return entry->d_type == DT_REG || entry->d_type == DT_LNK;
    } else {
        return entry->d_type == DT_REG;
    }
}

DIR *open_dir_or_queue_file(file_queue *queue, const char *dir_path)
{
    // Open the path as a directory. If it is not a directory, check whether if it is a file,
    // and then add the file to the queue if it is true.
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        int opendir_errno = errno;
        struct stat st;
        if (stat(dir_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (opendir_errno == EMFILE) {
                    log_w("Too many open files (%s). You might want to increase fd limit by `ulimit -n N`.", dir_path);
                } else {
                    log_w("%s can't be opend (errno: %d). You might want to search under the directory again.", opendir_errno, dir_path);
                }
            } else {
                enqueue_file_exclusively(queue, dir_path);
            }
        } else {
            log_w("'%s' can't be opened. Is there the directory or file on your current directory?", dir_path);
        }
    }

    return dir;
}

/**
 * Find search target files recursively under the specified directories, and add filenames to the
 * queue used by search worker.
 */
void scan_target(file_queue *queue, const char *dir_path, ignore_hash *ignores, int depth)
{
    DIR *dir = open_dir_or_queue_file(queue, dir_path);
    if (dir == NULL) {
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
        sprintf(buf, "%s%s", base, ".gitignore");
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
        if (op.follow_link && entry->d_type == DT_LNK) {
            char link[MAX_PATH_LENGTH] = { 0 };
            readlink(buf, link, MAX_PATH_LENGTH);
            if (access(link, F_OK) != 0) {
                continue;
            }
        }

        if (entry->d_type == DT_DIR) {
            scan_target(queue, buf, ignores, depth + 1);
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
}
