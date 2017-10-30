#ifndef util_h
#define util_h

#define NO_CLEANUP NULL
#define NO_ARG NULL
#define DEFAULT_ATTR NULL
#define SUCCESS 0


/*
 * Function calls cleanup_routine with argument arg.
 * Then it terminates process with specified code.
 */
void exit_with_cleanup(int code, void (*cleanup_routine)(void*), void* arg);

/*
 * Function terminates the process if code is non-zero.
 */
void exit_if_error(int code);

/*
 * Function prints error message to stderr in format
 * <message>. <system_message>
 * where system_message is a string corresponding to
 * error_code.
 */
void log_error(const char* message, int error_code);

/*
 * If error_code argument is non-zero this function
 * will print error message to stderr in format
 * <message>. <system_message>
 * where system_message is a string corresponding to
 * error_code.
 */
void log_if_error(int error_code, const char* message);

/*
 * Function prints custom error message to stderr.
 * Then it terminates process with specified code.
 */
void exit_with_custom_message(const char* message, int code);

/*
 * Function checks if code is non-zero.
 * If so it calls cleanup_routine with argument arg
 * and terminates the process.
 * Otherwise returns back.
 */
// void exit_if_error(int code, void (*cleanup_routine)(void*), void* arg);

/*
 * Function checks if expression is non-zero.
 * If so it prints message to stderr and terminates the process.
 * Otherwise returns back.
 */
void exit_if_true_with_message(int expression, char* message);

#endif /* util_h */
