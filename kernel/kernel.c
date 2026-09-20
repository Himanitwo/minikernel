#include "terminal.h"
#include "interrupts.h"
#include "memory.h"
#include "paging.h"
#include "process.h"
#include "scheduler.h"
#include "../shell/shell.h"

void kernel_main()
{
    terminal_initialize();

    terminal_write("MINI KERNEL\n");
    terminal_write("================\n");

    interrupts_initialize();
    interrupt_software_test();
    terminal_write("Timer initialized at 100 Hz\n");
    memory_initialize();
    process_initialize();
    scheduler_initialize();

    terminal_write("Kernel initialized successfully\n");

    shell_start();

    while (1)
    {
        __asm__ volatile("hlt");
    }
}