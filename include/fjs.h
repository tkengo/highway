#ifndef _HW_FJS_H_
#define _HW_FJS_H_

#include "file.h"

void prepare_fjs(const char *pattern, int pattern_len, enum file_type t);
void free_fjs();
bool fjs(const char *buf, int search_len, const char *pattern, int pattern_len, enum file_type t, match *mt);

#endif // _HW_FJS_H_
