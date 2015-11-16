#ifndef HW_WORKER_H
#define HW_WORKER_H

#include <pthread.h>
#include "common.h"
#include "file_queue.h"
#include "option.h"

typedef struct _worker_params {
    int index;
    file_queue *queue;
} worker_params;

extern pthread_mutex_t file_mutex;
extern pthread_mutex_t print_mutex;
extern pthread_mutex_t print_immediately_mutex;
extern pthread_cond_t file_cond;
extern pthread_cond_t print_cond;
extern pthread_cond_t print_immediately_cond;

extern bool is_complete_scan_file();

bool init_mutex();
void destroy_mutex();
void *print_worker(void *arg);
void *search_worker(void *arg);

#endif // HW_WORKER_H
