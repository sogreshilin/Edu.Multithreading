#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../utils/util.h"

const int MUTEX_COUNT = 3;
const int LINES_COUNT = 10;
const int NO_INITIALIZED_MUTEXES = 0;

pthread_mutex_t global_mutexes[MUTEX_COUNT];

/*****************************************************************************
 * Cleanup data and cleanup routine.
 ****************************************************************************/

typedef struct cleanup_data_s {
    pthread_mutexattr_t* attributes;
    size_t initialized_mutex_count;

} cleanup_data_t;

void cleanup_routine(void* arg) {
    cleanup_data_t* cleanup_data = (cleanup_data_t*) arg;
    if (cleanup_data->attributes != NULL) {
        printf("destroy attributes\n");
        pthread_mutexattr_destroy(cleanup_data->attributes);
    }
    for (int i = 0; i < cleanup_data->initialized_mutex_count; ++i) {
        printf("desroy global_mutexes[%d]\n", i);
        pthread_mutex_destroy(global_mutexes + i);
    }
}

/*****************************************************************************
 * Threads routine functions.
 ****************************************************************************/

void print_text(char* string, int start_index) {
    for (int i = 0; i < LINES_COUNT; ++i) {
        pthread_mutex_lock(global_mutexes + (i + start_index) % 3);
        printf("line %d from %s thread\n", i, string);
        pthread_mutex_unlock(global_mutexes + (i + start_index + 2) % 3);
    }
}

void* thread_routine(void* arg) {
    pthread_mutex_lock(global_mutexes + 2);
    print_text((char*) arg, 0);
    return NO_RETURN_VALUE;
}



int main() {
    pthread_mutexattr_t attributes;
    int code = pthread_mutexattr_init(&attributes);
    if (code != SUCCESS) {
        exit_with_custom_message("Unable to initialize mutex \
            attributes object", code);
    }
    cleanup_data_t cleanup_data = { &attributes, NO_INITIALIZED_MUTEXES };
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_ERRORCHECK);

    for (int i = 0; i < MUTEX_COUNT; ++i) {
        code = pthread_mutex_init(global_mutexes + i, &attributes);
        if (code != SUCCESS) {
            log_error("Unable to initialize mutex", code);
            exit_with_cleanup(code, cleanup_routine, (void*) &cleanup_data);
        }
        ++cleanup_data.initialized_mutex_count;
    }

    pthread_mutex_lock(global_mutexes + 0);

    pthread_t child_thread;
    code = pthread_create(&child_thread, DEFAULT_ATTR, thread_routine, (void*) "child");
    if (code != SUCCESS) {
        pthread_mutex_unlock(global_mutexes + 0);
        log_error("Unable to start thread", code);
        exit_with_cleanup(code, cleanup_routine, (void*) &cleanup_data);
    }

    while (!pthread_mutex_trylock(global_mutexes + 2)) {
        pthread_mutex_unlock(global_mutexes + 2);
    }

    print_text("parent", 1);

    code = pthread_join(child_thread, NO_ARG);
    if (code != SUCCESS) {
        log_error("Unable to join thread", code);
        exit_with_cleanup(code, cleanup_routine, (void*) &cleanup_data);
    }

    cleanup_routine((void*) &cleanup_data);
    exit(SUCCESS);
}
