#ifndef _HW_FILE_H_
#define _HW_FILE_H_

#include <dirent.h>
#include "common.h"

#define BUF_SIZE_FOR_FILE_TYPE_CHECK 512
#define AVAILABLE_ENCODING_COUNT 3
#define MAX_PATH_LENGTH 1024

enum file_type {
    FILE_TYPE_BINARY = -1,
    FILE_TYPE_SHIFT_JIS,
    FILE_TYPE_EUC_JP,
    FILE_TYPE_UTF8
};

#include "file_queue.h"

enum file_type detect_file_type(int fd, const char *filename);

#endif // _HW_FILE_H_
