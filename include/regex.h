#ifndef _HW_REGEX_H_
#define _HW_REGEX_H_

#include "common.h"
#include "file.h"
#include "oniguruma.h"

bool onig_init_wrap();
void onig_end_wrap();
regex_t *onig_new_wrap(const char *pattern, enum file_type t, bool ignore_case);

#endif // _HW_REGEX_H_
