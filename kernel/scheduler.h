#ifndef SCHEDULER_H
#define SCHEDULER_H

void scheduler_initialize();
void scheduler_run();
int scheduler_current_pid();
unsigned int scheduler_switch(unsigned int current_stack);
void scheduler_process_exited();
void scheduler_on_tick(unsigned int tick);

#endif