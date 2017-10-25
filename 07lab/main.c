#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "../utils/util.h"

const int ITER_COUNT = 2000000000;
const int EXPECTED_ARGS_COUNT = 2;
const int BASE = 0;

typedef struct threads_data_s {
    int thread_id;
    int thread_count;
    double result;
} threads_data_t;

threads_data_t* allocate_threads_data(int thread_count) {
    return (threads_data_t*)malloc(thread_count * sizeof(threads_data_t));
}

void free_threads_data(void* ptr) {
    free(ptr);
}
void fill_threads_data(threads_data_t* threads_data, int thread_count) {
    for (int i = 0; i < thread_count; ++i) {
        threads_data[i].thread_id = i;
        threads_data[i].thread_count = thread_count;
    }
}


void* compute_pi(void *arg) {
    threads_data_t *thread_data = (threads_data_t*)arg;
    int id = thread_data->thread_id;
    int num_threads = thread_data->thread_count;
    double result = 0;
    for (int index = id; index < ITER_COUNT; index += num_threads) {
        result += 1.0/(index * 4.0 + 1.0);
        result -= 1.0/(index * 4.0 + 3.0);
    }
    thread_data->result = result;
    pthread_exit(NULL);
}


void print_results(threads_data_t* data, int thread_count, double pi) {
    for (int i = 0; i < thread_count; ++i) {
        printf("[thread-%d]: result = %.15f\n", data[i].thread_id,
               data[i].result);
    }
    printf("--------------\n");
    printf("pi = %.15f\n", pi);
}


int start_all_threads(pthread_t* threads, threads_data_t* data, int thread_count) {
    for (int i = 0; i < thread_count; ++i) {
        int code = pthread_create(threads + i, DEFAULT_ATTR, compute_pi,
                                  (void*)(data + i));
        if (code != 0) {
            return code;
        }
    }
    return EXIT_SUCCESS;
}


int parse_arg_or_exit_if_error(int argc, const char *argv[]) {
    exit_if_true_with_message(argc < EXPECTED_ARGS_COUNT,
        "Thread count was expected as a parameter");
    char* endptr;
    int thread_count = (int)strtol(argv[1], &endptr, BASE);
    exit_if_error(errno, NO_CLEANUP, NO_ARG);
    exit_if_true_with_message(endptr[0] != '\0', "Invalid input");
    exit_if_true_with_message(thread_count <= 0,
        "Thread count parameter must be a positive number");
    return thread_count;
}


int main(int argc, const char *argv[]) {
    /* Reading thread_count passed as command line argument */
    int thread_count = parse_arg_or_exit_if_error(argc, argv);

    /* Allocating memory for data to pass to each thread routine
     * and initializing this data */
    threads_data_t *threads_data = allocate_threads_data(thread_count);
    exit_if_error(threads_data == NULL ? ENOMEM : 0, NO_CLEANUP, NO_ARG);
    fill_threads_data(threads_data, thread_count);

    /* Creating an array of thread descriptors,
     * starting all threads and checking if
     * there was an error while creating */
    pthread_t threads[thread_count];
    int code = start_all_threads(threads, threads_data, thread_count);
    exit_if_error(code, free_threads_data, threads_data);

    /* Joining threads and sum up their results */
    double pi = 0;
    for (int i = 0; i < thread_count; ++i) {
        code = pthread_join(threads[i], NULL);
        exit_if_error(code, free_threads_data, threads_data);
        pi += threads_data[i].result * 4;
    }

    /* Printing results of each thread and total */
    print_results(threads_data, thread_count, pi);

    /* Freeing allocated memory and terminating the process */
    free_threads_data(threads_data);
    exit(EXIT_SUCCESS);
}
