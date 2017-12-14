#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "../utils/util.h"

#define SEM_PRIVATE 0
#define SEM_INIT_VALUE 0
#define PRODUCER_COUNT 5
#define A_DETAIL_TIMEOUT 1
#define B_DETAIL_TIMEOUT 2
#define C_DETAIL_TIMEOUT 3

#define SEM_COUNT 4
sem_t detail_a;
sem_t detail_b;
sem_t detail_c;
sem_t module;

typedef enum { RUNNING, STOPPED } program_state_t;
program_state_t global_state = RUNNING;

void set_global_state(program_state_t state) {
    global_state = state;
}

void handle_signal(int signal_number) {
    if (signal_number == SIGINT) {
        set_global_state(STOPPED);
        sem_post(&detail_a);
        sem_post(&detail_b);
        sem_post(&detail_c);
        sem_post(&module);
    }
}

int set_signal_handler() {
   errno = 0;
   if (signal(SIGINT, handle_signal) == SIG_ERR) {
       return errno;
   }
   return SUCCESS;
}

void cleanup_routine(void* arg) {
    sem_destroy(&detail_a);
    sem_destroy(&detail_b);
    sem_destroy(&detail_c);
    sem_destroy(&module);
}

void* produce_simple_detail(const char* detail_name,
        int producing_timeout, sem_t* detail) {
    int detail_id = 0;
    sleep(producing_timeout);
    while (global_state == RUNNING) {
        sem_post(detail);
        printf("detail %s-%d produced\n", detail_name, detail_id);
        detail_id++;
        sleep(producing_timeout);
    }
    pthread_exit(NO_RETURN_VALUE);
}

void* produce_detail_a(void* arg) {
    return produce_simple_detail("A", A_DETAIL_TIMEOUT, &detail_a);
}

void* produce_detail_b(void* arg) {
    return produce_simple_detail("B", B_DETAIL_TIMEOUT, &detail_b);
}

void* produce_detail_c(void* arg) {
    return produce_simple_detail("C", C_DETAIL_TIMEOUT, &detail_c);
}

void* produce_module(void* arg) {
    int detail_a_id = 0;
    int detail_b_id = 0;
    int module_id = 0;
    sem_wait(&detail_a);
    sem_wait(&detail_b);
    while (global_state == RUNNING) {
        sem_post(&module);
        printf("module-%d produced from (A-%d, B-%d)\n",
            module_id, detail_a_id, detail_b_id);
        detail_a_id++;
        detail_b_id++;
        module_id++;
        sem_wait(&detail_a);
        sem_wait(&detail_b);
    }
    pthread_exit(NO_RETURN_VALUE);
}

void* produce_widget(void* arg) {
    int widget_id = 0;
    int module_id = 0;
    int detail_c_id = 0;
    sem_wait(&detail_c);
    sem_wait(&module);
    while (global_state == RUNNING) {
        printf("widget-%d produced from (C-%d, M-%d)\n",
            widget_id, detail_c_id, module_id);
        widget_id++;
        detail_c_id++;
        module_id++;
        sem_wait(&detail_c);
        sem_wait(&module);
    }
    pthread_exit(NO_RETURN_VALUE);
}

int initialize_semaphore(sem_t* semaphore) {
    int code = sem_init(semaphore, SEM_PRIVATE, SEM_INIT_VALUE);
    if (code != SUCCESS) {
        log_error("Unable to initialize semaphore", code);
    }
    return code;
}

int initialize_all_semaphores() {
    sem_t* semaphores[SEM_COUNT] = {&detail_a, &detail_b, &detail_c, &module};
    for (int i = 0; i < SEM_COUNT; ++i) {
        int code = sem_init(semaphores[i], SEM_PRIVATE, SEM_INIT_VALUE);
        if (code != SUCCESS) {
            return code;
        }
    }
    return SUCCESS;
}

int start_all_producers(pthread_t* producers,
        void* (*tasks[])(void*), int producer_count) {
    for (int i = 0; i < producer_count; ++i) {
        int code = pthread_create(producers + i, DEFAULT_ATTR, tasks[i],
            NO_ARG);
        if (code != SUCCESS) {
            return code;
        }
    }
    return SUCCESS;
}

int join_all_producers(pthread_t* producers, int producer_count) {
    int code = 0;
    for (int i = 0; i < producer_count; ++i) {
        code = pthread_join(producers[i], NO_RETURN_VALUE);
        if (code != SUCCESS) {
            return code;
        }
    }
    return SUCCESS;
}

int main() {
    int code = set_signal_handler();
    if (code != SUCCESS) {
        log_error("SIGINT handler was not set", code);
    }

    code = initialize_all_semaphores();
    if (code != SUCCESS) {
        log_error("Unable to initialize semaphore", code);
        exit_with_cleanup(code, cleanup_routine, NO_ARG);
    }

    void* (*tasks[PRODUCER_COUNT])(void* arg) = {
        produce_detail_a, produce_detail_b, produce_detail_c,
        produce_module, produce_widget
    };

    pthread_t producers[PRODUCER_COUNT];
    code = start_all_producers(producers, tasks, PRODUCER_COUNT);
    if (code != SUCCESS) {
        log_error("Unable to start producers", code);
        exit_with_cleanup(code, cleanup_routine, NO_ARG);
    }

    code = join_all_producers(producers, PRODUCER_COUNT);
    if (code != SUCCESS) {
        log_error("Unable to join producers", code);
        exit_with_cleanup(code, cleanup_routine, NO_ARG);
    }

    exit_with_cleanup(SUCCESS, cleanup_routine, NO_ARG);
}
