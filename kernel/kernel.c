#include "terminal.h"
#include "interrupts.h"
#include "memory.h"
#include "paging.h"
#include "process.h"
#include "scheduler.h"
#include "gdt.h"
#include "../shell/shell.h"

void kernel_main()
{
    terminal_initialize();

    terminal_write("MINI KERNEL\n");
    terminal_write("================\n");
    gdt_initialize();
    interrupts_initialize();

    terminal_write("Interrupts initialized\n");

    memory_initialize();

    terminal_write("Memory initialized\n");

    process_initialize();

    terminal_write("Processes initialized\n");

    scheduler_initialize();

    terminal_write("Scheduler initialized\n");

    terminal_write("Starting shell...\n");

    shell_start();

    while (1)
    {
        __asm__ volatile("hlt");
    }
}