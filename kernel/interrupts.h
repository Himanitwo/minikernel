#ifndef INTERRUPTS_H
#define INTERRUPTS_H

void interrupts_initialize();
void enable_interrupts();
void disable_interrupts();
unsigned char keyboard_read();
int keyboard_try_read();
void keyboard_interrupt_handler();
void exception_handler();
void interrupt_software_test();
void timer_interrupt_handler();
unsigned int timer_ticks();

#endif