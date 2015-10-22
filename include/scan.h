#ifndef HW_SCAN_H
#define HW_SCAN_H

#include "file_queue.h"
#include "ignore.h"

void scan_target(file_queue *queue, const char *dir_path, ignore_hash *ignores, int depth);

#endif // HW_SCAN_H
