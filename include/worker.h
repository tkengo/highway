#ifndef _HW_WORKER_H_
#define _HW_WORKER_H_

#include "common.h"
#include "file_queue.h"
#include "option.h"

typedef struct _worker_params {
    int index;
    file_queue *queue;
} worker_params;

extern pthread_mutex_t file_mutex;
extern pthread_mutex_t print_mutex;
extern pthread_cond_t file_cond;
extern pthread_cond_t print_cond;

extern bool is_complete_finding_file();

bool init_mutex();
void destroy_mutex();
void *print_worker(void *arg);
void *search_worker(void *arg);

#endif // _HW_WORKER_H_
