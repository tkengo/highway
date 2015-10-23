#ifndef HW_SCAN_H
#define HW_SCAN_H

#include "file_queue.h"
#include "ignore.h"

bool is_search_target_by_stat(const struct stat *st);
void scan_target(file_queue *queue, const char *dir_path, ignore_hash *ignores, int depth);
void enqueue_file_exclusively(file_queue *queue, const char *filename);

#endif // HW_SCAN_H
