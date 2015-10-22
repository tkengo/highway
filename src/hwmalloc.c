#include <stdlib.h>
#include "hwmalloc.h"
#include "log.h"

void *hw_malloc(size_t size)
{
    void *m = tc_malloc(size);
    if (m == NULL) {
        log_e("Memory allocation by malloc failed (%ld bytes).", size);
        exit(2);
    }
    return m;
}

void *hw_calloc(size_t count, size_t size)
{
    void *m = tc_calloc(count, size);
    if (m == NULL) {
        log_e("Memory allocation by calloc failed (%ld count, %ld bytes).", count, size);
        exit(2);
    }
    return m;
}

void *hw_realloc(void *ptr, size_t size)
{
    void *m = tc_realloc(ptr, size);
    if (m == NULL) {
        log_e("Memory allocation by realloc failed (%ld bytes).", size);
        exit(2);
    }
    return m;
}
