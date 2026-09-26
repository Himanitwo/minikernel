#ifndef SCHEDULER_H
#define SCHEDULER_H

void scheduler_initialize(void);

void scheduler_on_tick(
    unsigned int tick
);

unsigned int scheduler_switch(
    unsigned int current_stack
);

int scheduler_current_pid(void);

void scheduler_process_exited(void);

#endif