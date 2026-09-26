#include "scheduler.h"
#include "process.h"
#include "terminal.h"

extern Process processes[MAX_PROCESSES];

extern int process_count;

static int current_process = -1;

static int last_selected = -1;

static int scheduler_started = 0;

static unsigned int idle_stack = 0;


/*
 * Initialize scheduler.
 */
void scheduler_initialize()
{
    current_process = -1;

    last_selected = -1;

    scheduler_started = 0;

    idle_stack = 0;

    terminal_write(
        "Scheduler initialized\n"
    );
}


/*
 * Wake processes whose
 * wait period has expired.
 */
void scheduler_on_tick(unsigned int tick)
{
    int i;

    for (i = 0; i < MAX_PROCESSES; i++)
    {
        if (processes[i].state == WAITING &&
            processes[i].wake_tick <= tick)
        {
            processes[i].state = READY;
        }
    }
}


/*
 * Find next READY process.
 */
static int find_next_process(void)
{
    int offset;
    int candidate;

    for (offset = 1; offset <= MAX_PROCESSES; offset++)
    {
        candidate = (last_selected + offset) % MAX_PROCESSES;

        if (processes[candidate].state == READY &&
            processes[candidate].entry != 0)
        {
            return candidate;
        }
    }

    return -1;
}

/*
 * Context switch.
 */
unsigned int scheduler_switch(unsigned int current_stack)
{
    int next;

    if (!scheduler_started)
    {
        scheduler_started = 1;
        idle_stack = current_stack;
    }

    /*
     * Save the currently running process.
     */
    if (current_process >= 0)
    {
        processes[current_process].stack_pointer = current_stack;

        if (processes[current_process].state == RUNNING)
            processes[current_process].state = READY;

        last_selected = current_process;
    }

    /*
     * Immediately find the next READY process.
     */
    next = find_next_process();

    if (next < 0)
    {
        current_process = -1;
        return idle_stack;
    }

    current_process = next;

    processes[current_process].state = RUNNING;
    processes[current_process].started = 1;

    return processes[current_process].stack_pointer;
}


/*
 * Return current PID.
 */
int scheduler_current_pid()
{
    return current_process;
}


/*
 * Tell scheduler current
 * process has exited.
 */
void scheduler_process_exited()
{
    if (current_process >= 0)
    {
        processes[current_process]
            .state =
            TERMINATED;

        last_selected =
            current_process;

        current_process = -1;
    }
}