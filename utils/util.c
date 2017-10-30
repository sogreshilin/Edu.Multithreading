#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

void exit_with_cleanup(int code, void (*cleanup_routine)(void*), void* arg) {
    if (cleanup_routine != NO_CLEANUP) {
        cleanup_routine(arg);
    }
    exit(code);
}

void exit_if_error(int code) {
    if (code != EXIT_SUCCESS) {
        fprintf(stderr, "%s\n", strerror(code));
        exit(code);
    }
}

void log_error(const char* message, int error_code) {
    fprintf(stderr, "%s. %s\n", message, strerror(error_code));
}

void log_if_error(int error_code, const char* message) {
    if (error_code != SUCCESS) {
        log_error(message, error_code);
    }
}

void exit_with_custom_message(const char* message, int code) {
    fprintf(stderr, "%s\n", message);
    exit(code);
}


// void exit_if_error(int code, void (*cleanup_routine)(void*), void* arg) {
//     if (code != 0) {
//         const char* error_msg = strerror(code);
//         fprintf(stderr, "error #%d : %s\n", code, error_msg);
//         if (cleanup_routine != NO_CLEANUP) {
//             cleanup_routine(arg);
//         }
//         exit(EXIT_FAILURE);
//     }
// }

void exit_if_true_with_message(int expression, char* message) {
    if (expression != 0) {
        fprintf(stderr, "%s\n", message);
        exit(EXIT_FAILURE);
    }
}
