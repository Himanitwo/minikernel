#ifndef SCHEDULER_H
#define SCHEDULER_H

void scheduler_initialize();

unsigned int scheduler_switch(
    unsigned int current_stack
);

void scheduler_on_tick(
    unsigned int tick
);

int scheduler_current_pid();

void scheduler_process_exited();

#endif