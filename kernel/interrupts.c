#include "interrupts.h"
#include "terminal.h"

void interrupts_initialize()
{
    terminal_write("Interrupt system initialized\n");
}

void enable_interrupts()
{
    __asm__ volatile("sti");
}

void disable_interrupts()
{
    __asm__ volatile("cli");
}