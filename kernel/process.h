#ifndef PROCESS_H
#define PROCESS_H

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
} Process;

void process_initialize();
int create_process(char *name);

#endif