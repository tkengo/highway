#ifndef HW_REGEX_H
#define HW_REGEX_H

#include "common.h"
#include "file.h"
#include "oniguruma.h"

bool onig_init_wrap();
void onig_end_wrap();
regex_t *onig_new_wrap(const char *pattern, enum file_type t, bool ignore_case, int thread_no);
bool regex(const char *buf, size_t search_len, const char *pattern, enum file_type t, match *m, int thread_no);

#endif // HW_REGEX_H
