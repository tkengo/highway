#ifndef _HW_WORKER_H_
#define _HW_WORKER_H_

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
void destroy_mutex();
void print_to_terminal(const char *filename, file_queue_node *current, const hw_option *op);
void print_redirection(const char *filename, file_queue_node *current, const hw_option *op);
void *print_worker(void *arg);
void *search_worker(void *arg);

#endif // _HW_WORKER_H_
