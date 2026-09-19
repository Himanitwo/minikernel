#include "process.h"
#include "terminal.h"

Process processes[10];
int process_count = 0;

void process_initialize()
{
    terminal_write("Process manager initialized\n");
}

int create_process(char *name)
{
    int pid = process_count;

    processes[pid].pid = pid;
    processes[pid].state = READY;

    process_count++;

    terminal_write("Process created\n");

    return pid;
}