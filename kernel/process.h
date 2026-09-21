#ifndef PROCESS_H
#define PROCESS_H

#define MAX_PROCESSES 10

typedef enum
{
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} ProcessState;

typedef struct
{
    int pid;

    ProcessState state;

    char name[32];

    void (*entry)(void);

    unsigned int stack_pointer;
    unsigned int stack_top;

    unsigned int wake_tick;

    int started;

} Process;


extern Process processes[MAX_PROCESSES];


void process_initialize(void);

int create_process_with_pid(
    int pid,
    char *name,
    void (*entry)(void)
);

int create_process_with_entry(
    char *name,
    void (*entry)(void)
);

Process *process_get(int pid);

void process_exit_current(void);

int process_terminate(int pid);

int process_wait(
    int pid,
    unsigned int ticks
);

void process_yield_current(void);

#endif