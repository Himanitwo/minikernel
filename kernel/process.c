#include "process.h"
#include "terminal.h"

Process processes[MAX_PROCESSES];
int process_count = 0;
static unsigned char process_stacks[MAX_PROCESSES][PROCESS_STACK_SIZE]
    __attribute__((aligned(16)));
extern void process_return_trampoline();

void process_initialize()
{
    terminal_write("Process manager initialized\n");
}

int create_process(char *name)
{
    return create_process_with_entry(name, 0);
}

int create_process_with_entry(char *name, void (*entry)(void))
{
    int character;
    int pid;

    for (pid = 0; pid < process_count; pid++)
    {
        if (processes[pid].state == TERMINATED)
            break;
    }

    if (pid == process_count && process_count < MAX_PROCESSES)
        process_count++;

    if (pid >= MAX_PROCESSES)
    {
        terminal_write("Process limit reached\n");
        return -1;
    }

    processes[pid].pid = pid;
    processes[pid].state = READY;
    processes[pid].entry = entry;
    processes[pid].stack_top =
        (unsigned int)&process_stacks[pid][PROCESS_STACK_SIZE];
    processes[pid].stack_pointer = processes[pid].stack_top - 48;
    processes[pid].started = 0;
    processes[pid].wake_tick = 0;

    if (entry != 0)
    {
        unsigned int *stack =
            (unsigned int *)processes[pid].stack_pointer;

        stack[8] = (unsigned int)entry;
        stack[9] = 0x10;
        stack[10] = 0x202;
        stack[11] = (unsigned int)process_return_trampoline;
    }

    for (character = 0; character < 19 && name[character] != '\0'; character++)
        processes[pid].name[character] = name[character];

    processes[pid].name[character] = '\0';
    terminal_write("Process created\n");

    return pid;
}

Process *process_get(int pid)
{
    if (pid < 0 || pid >= process_count)
        return 0;

    return &processes[pid];
}

void process_exit_current()
{
    extern int scheduler_current_pid();
    process_terminate(scheduler_current_pid());
}

int process_terminate(int pid)
{
    extern void scheduler_process_exited();
    Process *process = process_get(pid);

    if (process == 0 || process->state == TERMINATED)
        return -1;

    process->state = TERMINATED;

    if (process->entry != 0)
        scheduler_process_exited();

    return 0;
}

int process_wait(int pid, unsigned int duration)
{
    extern unsigned int timer_ticks();
    Process *process = process_get(pid);

    if (process == 0 || process->state == TERMINATED)
        return -1;

    process->wake_tick = timer_ticks() + duration;
    process->state = WAITING;
    return 0;
}

void process_yield_current()
{
    extern int scheduler_current_pid();
    Process *process = process_get(scheduler_current_pid());

    if (process != 0 && process->state == RUNNING)
        process->state = READY;
}