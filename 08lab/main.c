#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include "../utils/util.h"

const int ITER_COUNT = 2e7;
const int MIN_ITER_COUNT = 1e6;
const int ARGS_REQUIRED = 2;
const int BASE = 0;

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t global_barrier;

/*****************************************************************************
 * Program global state functions.
 ****************************************************************************/

typedef enum { RUNNING, STOPPED } program_state_t;
program_state_t global_state = RUNNING;

void set_global_state(program_state_t state) {
    global_state = state;
}

/*****************************************************************************
 * Global data. Setter is a critical section.
 ****************************************************************************/

int global_max_iter;

void set_global_max_iter_if_greater(int iter_count) {
    pthread_mutex_lock(&global_mutex);
    if (iter_count > global_max_iter) {
        global_max_iter = iter_count;
    }
    pthread_mutex_unlock(&global_mutex);
}

int get_global_max_iter() {
    pthread_mutex_lock(&global_mutex);
    int max_iter = global_max_iter;
    pthread_mutex_unlock(&global_mutex);
    return max_iter;
}

/*****************************************************************************
 * Threads data type: constructor, setter and destructor.
 ****************************************************************************/

typedef struct thread_workload_s {
    int thread_id;
    int thread_count;
    double result;
} thread_workload_t;

thread_workload_t* allocate_threads_workload(int thread_count) {
    return (thread_workload_t*)malloc(thread_count * sizeof(thread_workload_t));
}

void free_threads_workload(void* ptr) {
    free(ptr);
}

void fill_threads_workload(thread_workload_t* threads_workload, int thread_count) {
    for (int i = 0; i < thread_count; ++i) {
        threads_workload[i].thread_id = i;
        threads_workload[i].thread_count = thread_count;
    }
}

/*****************************************************************************
 * Threads routine functions.
 ****************************************************************************/

double finish_computing_pi(int iter_count, thread_workload_t* thread_workload) {
    set_global_max_iter_if_greater(iter_count);
    pthread_barrier_wait(&global_barrier);
    int id = thread_workload->thread_id;
    int num_threads = thread_workload->thread_count;
    int index = id + iter_count * num_threads;
    double result = 0;
    double local_max_iter = get_global_max_iter();
    for ( ;iter_count < local_max_iter; ++iter_count) {
        result += 1.0/(index * 4.0 + 1.0);
        result -= 1.0/(index * 4.0 + 3.0);
        index += num_threads;
    }

    return result;
}

void* compute_pi(void *arg) {
    thread_workload_t *thread_workload = (thread_workload_t*)arg;
    int id = thread_workload->thread_id;
    int num_threads = thread_workload->thread_count;
    const int MAX_ITER_COUNT = INT_MAX / num_threads;

    int iter_count = 0;
    double result = 0;

    for (int index = id; ; index += num_threads) {
        ++iter_count;

        if (iter_count == MAX_ITER_COUNT) {
            printf("%.15f\n", index * 4.0 + 3.0);
        }
        if ((global_state == STOPPED && iter_count % MIN_ITER_COUNT == 0) ||
                (iter_count == MAX_ITER_COUNT)) {
            result += finish_computing_pi(iter_count, thread_workload);
            thread_workload->result = result;
            pthread_exit(arg);
        }

        result += 1.0/(index * 4.0 + 1.0);
        result -= 1.0/(index * 4.0 + 3.0);

    }
}

/*****************************************************************************
 * Util functions.
 ****************************************************************************/

 void handle_signal(int signal_number) {
     if (signal_number == SIGINT) {
         set_global_state(STOPPED);
     }
 }

int set_signal_handler() {
    errno = 0;
    if (signal(SIGINT, handle_signal) == SIG_ERR) {
        return errno;
    }
    return SUCCESS;
}

void print_usage() {
    printf("Usage: <program_name> <thread_count>\n");
}

int parse_thread_count(const char* string_value, int* result) {
    char* end_pointer;
    errno = 0;
    int thread_count = strtol(string_value, &end_pointer, BASE);
    if (errno == EINVAL || errno == ERANGE) {
        return EINVAL;
    }
    if (*end_pointer != '\0') {
        return EINVAL;
    }
    *result = (int) thread_count;
    return SUCCESS;
}

/*****************************************************************************
 * Threads managing functions: start, gather.
 ****************************************************************************/

int start_all_threads(pthread_t* threads, thread_workload_t* thread_workload,
        int thread_count) {
    int code = set_signal_handler();
    if (code != SUCCESS) {
        log_error("Unable to set signal handler", code);
        return code;
    }
    for (int i = 0; i < thread_count; ++i) {
        code = pthread_create(threads + i, DEFAULT_ATTR, compute_pi,
                                  (void*)(thread_workload + i));
        if (code != SUCCESS) {
            return code;
        }
    }
    return SUCCESS;
}

int gather_pi_value(pthread_t* threads, int thread_count, double* result) {
    double pi = 0;
    for (int i = 0; i < thread_count; ++i) {
        thread_workload_t* threads_workload;
        int code = pthread_join(threads[i], (void*)&threads_workload);
        if (code != SUCCESS) {
            return code;
        }
        pi += threads_workload->result;
    }
    pi *= 4;
    *result = pi;
    return SUCCESS;
}

/*****************************************************************************
 * Cleanup data and cleanup routine.
 ****************************************************************************/

typedef struct cleanup_data_s {
    pthread_barrier_t* barrier;
    thread_workload_t* threads_workload;
} cleanup_data_t;

void cleanup_routine(void* arg) {
    cleanup_data_t* cleanup_data = (cleanup_data_t*) arg;
    int code;
    if (cleanup_data->barrier != NULL) {
        code = pthread_barrier_destroy(cleanup_data->barrier);
        log_if_error(code, "Unable to destroy barrier\n");
    }
    if (cleanup_data->threads_workload != NULL) {
        free_threads_workload(cleanup_data->threads_workload);
    }
}

int prepare_data(int thread_count, thread_workload_t** threads_workload,
        cleanup_data_t* cleanup_data) {
    int code = pthread_barrier_init(&global_barrier, DEFAULT_ATTR, thread_count);
    if (code != SUCCESS) {
        log_error("Unable to initialize barrier", code);
        return code;
    }
    cleanup_data->barrier = &global_barrier;

    *threads_workload = allocate_threads_workload(thread_count);
    if (threads_workload == NULL) {
        log_error("Unable to allocate threads data", ENOMEM);
        return FAILURE;
    }
    cleanup_data->threads_workload = *threads_workload;
    fill_threads_workload(*threads_workload, thread_count);
    return SUCCESS;
}


int main(int argc, const char *argv[]) {
    if (argc != ARGS_REQUIRED) {
        print_usage();
        exit(EXIT_FAILURE);
    }
    int thread_count;
    int code = parse_thread_count(argv[1], &thread_count);
    if (code != SUCCESS) {
        exit_with_custom_message("Thread count parameter must be \
            a positive number", EXIT_FAILURE);
    }

    cleanup_data_t cleanup_data;
    thread_workload_t *threads_workload;
    code = prepare_data(thread_count, &threads_workload, &cleanup_data);
    if (code != SUCCESS) {
        exit_with_cleanup(EXIT_FAILURE, cleanup_routine, (void*) &cleanup_data);
    }

    pthread_t threads[thread_count];
    code = start_all_threads(threads, threads_workload, thread_count);
    if (code != SUCCESS) {
        log_error("Unable to start threads", code);
        exit_with_cleanup(EXIT_FAILURE, cleanup_routine, (void*) &cleanup_data);
    }

    double pi = 0;
    code = gather_pi_value(threads, thread_count, &pi);
    if (code != SUCCESS) {
        log_error("Unable to join threads", code);
        exit_with_cleanup(EXIT_FAILURE, cleanup_routine, (void*) &cleanup_data);
    }

    printf("\npi = %.15f\n", pi);
    exit_with_cleanup(EXIT_SUCCESS, cleanup_routine, (void*) &cleanup_data);
}
