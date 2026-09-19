#include "scheduler.h"
#include "process.h"
#include "terminal.h"

extern Process processes[10];
extern int process_count;

int current_process = 0;

void scheduler_initialize()
{
    terminal_write("Scheduler initialized\n");
}

void scheduler_run()
{
    if (process_count == 0)
        return;

    processes[current_process].state = RUNNING;

    terminal_write("Running process\n");

    processes[current_process].state = READY;

    current_process++;

    if (current_process >= process_count)
        current_process = 0;
}