#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void exit_if_error(int code, void (*cleanup_routine)(void*), void* arg) {
    if (code != 0) {
        const char* error_msg = strerror(code);
        fprintf(stderr, "error #%d : %s\n", code, error_msg);
        if (cleanup_routine != NO_CLEANUP) {
            cleanup_routine(arg);
        }
        exit(EXIT_FAILURE);
    }
}

void exit_if_true_with_message(int expression, char* message) {
    if (expression != 0) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}
