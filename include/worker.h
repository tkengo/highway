#ifndef _WORKER_H_
#define _WORKER_H_

#include "common.h"
#include "queue.h"

typedef struct _search_worker_params {
    file_queue *queue;
    char *pattern;
} search_worker_params;

extern pthread_mutex_t file_mutex;
extern pthread_mutex_t print_mutex;
extern pthread_cond_t file_cond;
extern pthread_cond_t print_cond;

extern bool is_complete_file_finding();

bool init_mutex();
void *print_worker(void *arg);
void *search_worker(void *arg);

#endif // _WORKER_H_
