#ifndef _WORKER_H_
#define _WORKER_H_

#include "common.h"
#include "queue.h"
#include "option.h"

typedef struct _worker_params {
    file_queue *queue;
    hw_option *op;
} worker_params;

extern pthread_mutex_t file_mutex;
extern pthread_mutex_t print_mutex;
extern pthread_cond_t file_cond;
extern pthread_cond_t print_cond;

extern bool is_complete_finding_file();

bool init_mutex();
void *print_worker(void *arg);
void *search_worker(void *arg);

#endif // _WORKER_H_
