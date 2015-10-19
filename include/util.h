#ifndef _HW_UTIL_H_
#define _HW_UTIL_H_

#ifndef _WIN32
#include <sys/resource.h>
#else
#include <windows.h>
#endif
#include "common.h"

#define MIN(a,b) ((a) < (b)) ? (a) : (b)
#define MAX(a,b) ((a) > (b)) ? (a) : (b)

#define IS_STDIN_REDIRECT (!isatty(STDIN_FILENO))
#define IS_STDOUT_REDIRECT (!isatty(STDOUT_FILENO))

#ifndef _WIN32
#define ENTRY_ISDIR(e) (e->d_type == DT_DIR)
#define IS_PATHSEP(c) (c == '/')
#else
#define ENTRY_ISDIR(e) (GetFileAttributes(e->d_name) & FILE_ATTRIBUTE_DIRECTORY)
#define IS_PATHSEP(c) (c == '/' || c == '\\')
typedef int rlim_t;
#endif

bool set_fd_rlimit(rlim_t limit);
bool is_word_sp(char c);
char *trim(char *str);
void init_iconv();
void close_iconv();
void to_euc(char *in, size_t nin, char *out, size_t nout);
void to_sjis(char *in, size_t nin, char *out, size_t nout);
void to_utf8_from_euc(char *in, size_t nin, char *out, size_t nout);
void to_utf8_from_sjis(char *in, size_t nin, char *out, size_t nout);

#endif // _HW_UTIL_H_
