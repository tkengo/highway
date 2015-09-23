#ifndef _HW_FILE_H_
#define _HW_FILE_H_

#include <dirent.h>
#include "common.h"

#define BUF_SIZE_FOR_FILE_TYPE_CHECK 512
#define AVAILABLE_ENCODING_COUNT 3

enum file_type {
    FILE_TYPE_BINARY = -1,
    FILE_TYPE_SHIFT_JIS,
    FILE_TYPE_EUC_JP,
    FILE_TYPE_UTF8
};

#include "queue.h"

enum file_type is_binary(int fd);
bool is_ignore_directory(struct dirent *entry);
bool is_directory(struct dirent *entry);
bool is_search_target(struct dirent *entry);

#endif // _HW_FILE_H_
