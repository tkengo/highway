#ifndef _HW_SCAN_H_
#define _HW_SCAN_H_

#include "file_queue.h"
#include "ignore.h"

void scan_target(file_queue *queue, const char *dir_path, ignore_hash *ignores, int depth);

#endif // _HW_SCAN_H_
