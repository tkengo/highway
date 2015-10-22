#ifndef HW_HWMALLOC_H
#define HW_HWMALLOC_H

#include "config.h"

#ifdef HAVE_LIBTCMALLOC
#include <gperftools/tcmalloc.h>
#else
#include <stdlib.h>
#define tc_malloc malloc
#define tc_calloc calloc
#define tc_free free
#endif

void *hw_malloc(size_t size);
void *hw_calloc(size_t count, size_t size);
void *hw_realloc(void *ptr, size_t size);

#endif // HW_HWMALLOC_H
