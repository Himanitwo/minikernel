#ifndef INTERRUPTS_H
#define INTERRUPTS_H

void interrupts_initialize(void);

void enable_interrupts(void);

void disable_interrupts(void);

unsigned char keyboard_read(void);

void keyboard_interrupt_handler(void);

void mouse_interrupt_handler(void);
void exception_handler(void);

void page_fault_handler(void);

void interrupt_software_test(void);

void timer_interrupt_handler(void);

unsigned int timer_ticks(void);

extern void isr_default(void);

extern void exception_stub(void);
extern void page_fault_stub(void);
extern void irq0_stub(void);

extern void irq1_stub(void);

extern void irq12_stub(void);

extern void software_test_stub(void);

#endif