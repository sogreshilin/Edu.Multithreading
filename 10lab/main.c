#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../utils/util.h"

const int MUTEX_COUNT = 3;
const int LINES_COUNT = 10;
const int NO_INITIALIZED_MUTEXES = 0;

const int PARENT_INITIAL_MUTEX_ID = 1;
const int CHILD_INITIAL_MUTEX_ID = 0;

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
        pthread_mutexattr_destroy(cleanup_data->attributes);
    }
    for (int i = 0; i < cleanup_data->initialized_mutex_count; ++i) {
        pthread_mutex_destroy(global_mutexes + i);
    }
}

/*****************************************************************************
 * Threads routine functions.
 ****************************************************************************/

/*
 * Thread calling this function has to have mutex with
 * id = (initial_mutex_id + MUTEX_COUNT - 1) % MUTEX_COUNT
 * already locked.
 *
 * Function does (to - from) iterations. On the i-th iteration it locks
 * (initial_mutex_id + i) mutex, prints text and unlocks locked mutex with
 * id = (initial_mutex_id + i + MUTEX_COUNT - 1) % MUTEX_COUNT.
 */
void print_text_synchronously(char* string, int initial_mutex_id, int from, int to) {
    for (int i = from; i < to; ++i) {
        int lock_id = (i + initial_mutex_id) % MUTEX_COUNT;
        int unlock_id = (i + initial_mutex_id + MUTEX_COUNT - 1) % MUTEX_COUNT;

        pthread_mutex_lock(global_mutexes + lock_id);
        printf("line %d from %s thread\n", i, string);
        pthread_mutex_unlock(global_mutexes + unlock_id);
    }
}

void* thread_routine(void* arg) {
    /*
     * Lock this mutex to let parent know that this child thread has started
     * its routine.
     */
    pthread_mutex_lock(global_mutexes + 2);
    print_text_synchronously((char*) arg, CHILD_INITIAL_MUTEX_ID, 0, LINES_COUNT);
    pthread_exit(NO_RETURN_VALUE);
}

/*****************************************************************************
 * Initializing mutexes.
 ****************************************************************************/

/*
 * Initializing MUTEX_COUNT error-check type mutexes.
 */
int initialize_mutexes(cleanup_data_t* cleanup_data) {
    pthread_mutexattr_t attributes;
    int code = pthread_mutexattr_init(&attributes);
    if (code != SUCCESS) {
        exit_with_custom_message("Unable to initialize mutex attributes object", code);
    }
    cleanup_data->attributes = &attributes;
    pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_ERRORCHECK);

    for (int i = 0; i < MUTEX_COUNT; ++i) {
        code = pthread_mutex_init(global_mutexes + i, &attributes);
        if (code != SUCCESS) {
            log_error("Unable to initialize mutex", code);
            return FAILURE;
        }
        cleanup_data->initialized_mutex_count = i + 1;
    }
    return SUCCESS;
}

int main() {
    cleanup_data_t cleanup_data;
    int code = initialize_mutexes(&cleanup_data);
    if (code != SUCCESS) {
        exit_with_cleanup(code, cleanup_routine, (void*) &cleanup_data);
    }

    /*
     * Lock this mutex to make sure that child won't start printing  before
     * parent. Child's first string will only be printed when child locks
     * 0 and 2 mutex.
     */
    pthread_mutex_lock(global_mutexes + 0);

    pthread_t child_thread;
    code = pthread_create(&child_thread, DEFAULT_ATTR, thread_routine, (void*) "child");

    /*
     * If there is an error in creating child thread we unlock all locked
     * mutexes, free all resources and terminate the process.
     */
    if (code != SUCCESS) {
        pthread_mutex_unlock(global_mutexes + 0);
        log_error("Unable to start thread", code);
        exit_with_cleanup(code, cleanup_routine, (void*) &cleanup_data);
    }


    /*
     * This cycle makes sure that child thread started his routine.
     */
    while (!pthread_mutex_trylock(global_mutexes + 2)) {
        pthread_mutex_unlock(global_mutexes + 2);
    }

    /*
     * Calling function with second argument PARENT_INITIAL_MUTEX_ID
     * which means that on the i-th iteration parent will lock
     * (PARENT_INITIAL_MUTEX_ID + i) mutex, print text and unlock
     * the other locked mutex with
     * id = (PARENT_INITIAL_MUTEX_ID + i + MUTEX_COUNT - 1) % MUTEX_COUNT.
     */
    print_text_synchronously("parent", PARENT_INITIAL_MUTEX_ID, 0, LINES_COUNT);

    code = pthread_join(child_thread, NO_ARG);
    if (code != SUCCESS) {
        log_error("Unable to join thread", code);
        exit_with_cleanup(code, cleanup_routine, (void*) &cleanup_data);
    }

    exit_with_cleanup(SUCCESS, cleanup_routine, (void*) &cleanup_data);
}
