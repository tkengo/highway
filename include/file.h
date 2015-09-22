#ifndef _HW_FILE_H_
#define _HW_FILE_H_

#include <dirent.h>
#include "common.h"
#include "queue.h"

bool is_binary(int fd);
bool is_ignore_directory(struct dirent *entry);
bool is_directory(struct dirent *entry);
bool is_search_target(struct dirent *entry);

#endif // _HW_FILE_H_
