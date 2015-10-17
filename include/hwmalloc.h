#ifndef _HW_HWMALLOC_H_
#define _HW_HWMALLOC_H_

// #include <stdlib.h>
#include <gperftools/tcmalloc.h>

// #define tc_malloc malloc
// #define tc_calloc calloc
// #define tc_free free
#define tc_malloc tc_malloc
#define tc_calloc tc_calloc
#define tc_free tc_free

#endif // _HW_HWMALLOC_H_
