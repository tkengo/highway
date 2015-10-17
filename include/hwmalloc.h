#ifndef _HW_HWMALLOC_H_
#define _HW_HWMALLOC_H_

#include "config.h"

#ifdef HAVE_LIBTCMALLOC
#include <gperftools/tcmalloc.h>
#else
#include <stdlib.h>
#define tc_malloc malloc
#define tc_calloc calloc
#define tc_free free
#endif

#endif // _HW_HWMALLOC_H_
