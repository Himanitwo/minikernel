#include "process.h"
#include "terminal.h"
#include "scheduler.h"
#include "interrupts.h"

typedef unsigned int uint32_t;

#define PROCESS_STACK_SIZE 4096

Process processes[MAX_PROCESSES];

static unsigned char process_stacks[MAX_PROCESSES][PROCESS_STACK_SIZE];

 int process_count = 0;


/*
 * This function is used as the return address of a process.
 *
 * If a process function ever returns, we mark the current
 * process as terminated and notify the scheduler.
 */
static void process_return_trampoline(void)
{
    process_exit_current();

    for (;;)
        __asm__ volatile("hlt");
}


/*
 * Initialize the process manager.
 */
void process_initialize(void)
{
    int i;

    process_count = 0;

    for (i = 0; i < MAX_PROCESSES; i++)
    {
        processes[i].pid = i;
        processes[i].state = TERMINATED;
        processes[i].name[0] = '\0';
        processes[i].entry = 0;
        processes[i].stack_pointer = 0;
        processes[i].stack_top = 0;
        processes[i].wake_tick = 0;
        processes[i].started = 0;
    }

    terminal_write(
        "Process manager initialized\n"
    );
}

/*
 * Create a process at a specific PID.
 *
 * The process stack is prepared so that the timer interrupt
 * context-switch code can restore it using:
 *
 *     mov esp, eax
 *     popa
 *     iretd
 */
int create_process_with_pid(
    int pid,
    char *name,
    void (*entry)(void))
{
    unsigned int *stack;

    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;

    if (processes[pid].state != TERMINATED)
        return -1;

    processes[pid].pid = pid;
    processes[pid].state = READY;
    processes[pid].entry = entry;
    processes[pid].wake_tick = 0;
    processes[pid].started = 0;

    /*
     * Copy process name.
     */
    {
        int i;

        for (i = 0; i < 31 && name[i] != '\0'; i++)
            processes[pid].name[i] = name[i];

        processes[pid].name[i] = '\0';
    }

    /*
     * Process stack grows downward.
     *
     * We reserve space for:
     *
     * 8 registers for POPA
     * 3 values for IRETD
     * 1 return address
     *
     * Total = 12 unsigned integers.
     */
    processes[pid].stack_top =
        (uint32_t)&process_stacks[pid][PROCESS_STACK_SIZE];

 processes[pid].stack_pointer =
    processes[pid].stack_top -
    (11 * sizeof(uint32_t));

    stack =
        (unsigned int *)processes[pid].stack_pointer;

    /*
     * ---------------------------------------------------------
     * POPA frame
     * ---------------------------------------------------------
     *
     * boot/interrupts.asm restores:
     *
     *     popa
     *
     * POPA expects:
     *
     * [0] EDI
     * [1] ESI
     * [2] EBP
     * [3] ESP (ignored by POPA)
     * [4] EBX
     * [5] EDX
     * [6] ECX
     * [7] EAX
     */
    stack[0] = 0;      /* EDI */
    stack[1] = 0;      /* ESI */
    stack[2] = 0;      /* EBP */
    stack[3] = 0;      /* ESP - ignored by POPA */
    stack[4] = 0;      /* EBX */
    stack[5] = 0;      /* EDX */
    stack[6] = 0;      /* ECX */
    stack[7] = 0;      /* EAX */


    /*
     * ---------------------------------------------------------
     * IRETD frame
     * ---------------------------------------------------------
     *
     * After POPA, IRETD expects:
     *
     * [8]  EIP
     * [9]  CS
     * [10] EFLAGS
     */

    stack[8] = (uint32_t)entry;

    /*
     * GDT:
     *
     * 0x00 = null descriptor
     * 0x08 = kernel CODE
     * 0x10 = kernel DATA
     */
    stack[9] = 0x08;

    /*
     * EFLAGS:
     *
     * 0x200 = Interrupt Enable Flag
     * 0x002 = Reserved bit
     */
    stack[10] = 0x202;


    /*
     * If the process function returns, RET will jump here.
     */
    processes[pid].stack_pointer =
    (uint32_t)&process_stacks[pid][PROCESS_STACK_SIZE]
    - (11 * sizeof(uint32_t));

    process_count++;

    return pid;
}


/*
 * Automatically allocate a free PID.
 */
int create_process_with_entry(
    char *name,
    void (*entry)(void))
{
    int pid;

    for (pid = 0;
         pid < MAX_PROCESSES;
         pid++)
    {
        if (processes[pid].state == TERMINATED)
        {
            return create_process_with_pid(
                pid,
                name,
                entry
            );
        }
    }

    return -1;
}


/*
 * Get a process by PID.
 */
Process *process_get(int pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
        return 0;

    if (processes[pid].state == TERMINATED)
        return 0;

    return &processes[pid];
}


/*
 * Terminate the currently running process.
 */
void process_exit_current(void)
{
    int pid;

    pid = scheduler_current_pid();

    if (pid < 0 || pid >= MAX_PROCESSES)
        return;

    processes[pid].state = TERMINATED;
    processes[pid].started = 0;

    scheduler_process_exited();
}


/*
 * Terminate a process using its PID.
 */
int process_terminate(int pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;

    if (processes[pid].state == TERMINATED)
        return -1;

    processes[pid].state = TERMINATED;
    processes[pid].started = 0;

    return 0;
}


/*
 * Wait for a process.
 *
 * The process is marked WAITING until the specified
 * number of timer ticks has passed.
 */
int process_wait(int pid, unsigned int ticks)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
        return -1;

    if (processes[pid].state == TERMINATED)
        return -1;

    processes[pid].state = WAITING;

processes[pid].wake_tick =
    timer_ticks() + ticks;

    return 0;
}


/*
 * Yield the current process.
 */
void process_yield_current(void)
{
    int pid;

    pid = scheduler_current_pid();

    if (pid < 0 || pid >= MAX_PROCESSES)
        return;

    if (processes[pid].state == RUNNING)
        processes[pid].state = READY;
}