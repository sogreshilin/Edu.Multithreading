#ifndef util_h
#define util_h

#define NO_CLEANUP NULL
#define NO_ARG NULL
#define DEFAULT_ATTR NULL

/*
 * Function checks if code is non-zero.
 * If so it calls cleanup_routine with argument arg
 * and terminates the process.
 * Otherwise returns back.
 */
void exit_if_error(int code, void (*cleanup_routine)(void*), void* arg);

/*
 * Function checks if expression is non-zero.
 * If so it prints message to stderr and terminates the process.
 * Otherwise returns back.
 */
void exit_if_true_with_message(int expression, char* message);

#endif /* util_h */
