#ifndef _FILE_H_
#define _FILE_H_

#include "highway.h"
#include "queue.h"

bool is_ignore_directory(struct dirent *entry);
bool is_directory(struct dirent *entry);
bool is_search_target(struct dirent *entry);
void find_target_files(file_queue *queue, char *dirname);

#endif // _FILE_H_
