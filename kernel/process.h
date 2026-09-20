#ifndef PROCESS_H
#define PROCESS_H

#define MAX_PROCESSES 10
#define PROCESS_STACK_SIZE 4096

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
    char name[20];
    void (*entry)(void);
    unsigned int stack_top;
    unsigned int stack_pointer;
    int started;
    unsigned int wake_tick;
} Process;

void process_initialize();
int create_process(char *name);
int create_process_with_entry(char *name, void (*entry)(void));
Process *process_get(int pid);
void process_exit_current();
int process_terminate(int pid);
int process_wait(int pid, unsigned int duration);
void process_yield_current();

#endif