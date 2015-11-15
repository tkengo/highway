#ifndef HW_PRINT_H
#define HW_PRINT_H

#include "line_list.h"
#include "file_queue.h"

void print_line(const char *filename, enum file_type t, match_line_node *match_line, int max_digit);
void print_result(file_queue_node *current);

#endif // HW_PRINT_H
