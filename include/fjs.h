#ifndef HW_FJS_H
#define HW_FJS_H

#include "file.h"

void prepare_fjs(const char *pattern, int pattern_len, enum file_type t);
void free_fjs();
bool fjs(const char *buf, size_t search_len, const char *pattern, int pattern_len, enum file_type t, match *mt);

#endif // HW_FJS_H
