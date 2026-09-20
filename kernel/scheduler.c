#include "scheduler.h"
#include "process.h"
#include "terminal.h"

extern Process processes[MAX_PROCESSES];
extern int process_count;

int current_process = -1;
int scheduler_started = 0;
unsigned int idle_stack = 0;

void scheduler_initialize()
{
    terminal_write("Scheduler initialized\n");
}

void scheduler_on_tick(unsigned int tick)
{
    int process_index;

    for (process_index = 0; process_index < process_count; process_index++)
    {
        if (processes[process_index].state == WAITING &&
            processes[process_index].wake_tick <= tick)
        {
            processes[process_index].state = READY;
        }
    }
}

void scheduler_run()
{
    int next_process;

    if (process_count == 0)
        return;

    if (current_process >= 0)
        processes[current_process].state = READY;

    for (next_process = 1; next_process <= process_count; next_process++)
    {
        int candidate = (current_process + next_process + process_count) %
            process_count;

        if (processes[candidate].state == READY)
        {
            current_process = candidate;
            processes[current_process].state = RUNNING;
            return;
        }
    }

    processes[current_process].state = RUNNING;
}

int scheduler_current_pid()
{
    if (current_process < 0 || process_count == 0)
        return -1;

    return processes[current_process].pid;
}

unsigned int scheduler_switch(unsigned int current_stack)
{
    int next_process;
    int candidate;

    if (process_count == 0)
        return current_stack;

    if (scheduler_started && current_process >= 0)
        processes[current_process].stack_pointer = current_stack;
    else
    {
        scheduler_started = 1;
        idle_stack = current_stack;
    }

    if (current_process >= 0 &&
        processes[current_process].state == RUNNING)
    {
        processes[current_process].state = READY;
        current_process = -1;
        return idle_stack;
    }

    for (next_process = 1; next_process <= process_count; next_process++)
    {
        candidate = (current_process + next_process + process_count) %
            process_count;

        if (processes[candidate].state == READY &&
            processes[candidate].entry != 0)
        {
            current_process = candidate;
            processes[current_process].state = RUNNING;
            processes[current_process].started = 1;
            return processes[current_process].stack_pointer;
        }
    }

    return current_stack;
}

void scheduler_process_exited()
{
    current_process = -1;
}