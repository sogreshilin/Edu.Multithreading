#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../utils/util.h"

const int TIMEOUT = 2;

void my_cleanup() {
    printf("child thread finished\n");
}

void* print_data(void* arg) {
    pthread_cleanup_push(my_cleanup, NULL);
    for (int i = 0; ; ++i) {
        printf("child thread printing %d line\n", i);
        pthread_testcancel();
    }

    /* Never reach this. Need because push and pop are macros */
    pthread_cleanup_pop(1);
    return NULL;
}

int main() {
    pthread_t thread;
    int code = pthread_create(&thread, DEFAULT_ATTR, print_data, NO_ARG);
    exit_if_error(code, NO_CLEANUP, NO_ARG);

    sleep(TIMEOUT);

    code = pthread_cancel(thread);
    exit_if_error(code, NO_CLEANUP, NO_ARG);


    code = pthread_join(thread, NULL);
    exit_if_error(code, NO_CLEANUP, NO_ARG);

    printf("parent thread finished\n");
    return EXIT_SUCCESS;

}
