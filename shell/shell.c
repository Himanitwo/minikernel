#include "shell.h"
#include "../kernel/terminal.h"
#include "../kernel/process.h"
#include "../kernel/interrupts.h"
#include "../kernel/scheduler.h"

static char command[50];
static int command_length = 0;

static void test_process()
{
    for (;;)
        __asm__ volatile("hlt");
}

static const char *process_state_name(ProcessState state)
{
    switch (state)
    {
        case READY: return "READY";
        case RUNNING: return "RUNNING";
        case WAITING: return "WAITING";
        case TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

static void write_decimal(int value)
{
    char digits[12];
    int length = 0;

    if (value == 0)
    {
        terminal_putchar('0');
        return;
    }

    if (value < 0)
    {
        terminal_putchar('-');
        value = -value;
    }

    while (value > 0)
    {
        digits[length] = '0' + (value % 10);
        value /= 10;
        length++;
    }

    while (length > 0)
        terminal_putchar(digits[--length]);
}

static char scancode_to_ascii(unsigned char scancode)
{
    switch (scancode)
    {
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';

        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';

        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';

        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';

        case 0x39: return ' ';

        default:
            return 0;
    }
}

static int command_equals(const char *a)
{
    int i = 0;

    while (a[i] != '\0' || command[i] != '\0')
    {
        if (a[i] != command[i])
            return 0;

        i++;
    }

    return 1;
}

static void execute_command()
{
    command[command_length] = '\0';

    terminal_write("\n");

    if (command_equals("help"))
    {
        terminal_write("Available commands:\n");
        terminal_write("help  - Show commands\n");
        terminal_write("clear - Clear screen\n");
        terminal_write("ps    - Show processes\n");
        terminal_write("mem   - Show memory\n");
        terminal_write("run   - Create process\n");
        terminal_write("wait  - Put process 0 to sleep for 500 ticks\n");
        terminal_write("kill  - Terminate process 0\n");
        terminal_write("ticks - Show timer ticks\n");
    }
    else if (command_equals("clear"))
    {
        terminal_initialize();
    }
    else if (command_equals("ps"))
    {
        extern Process processes[10];
        extern int process_count;
        int process_index;

        terminal_write("PID  STATE     NAME\n");
        for (process_index = 0; process_index < process_count; process_index++)
        {
            write_decimal(processes[process_index].pid);
            terminal_write("    ");
            terminal_write(process_state_name(processes[process_index].state));
            terminal_write("     ");
            terminal_write(processes[process_index].name);
            terminal_write("\n");
        }
    }
    else if (command_equals("mem"))
    {
        terminal_write("Memory Manager\n");
        terminal_write("Heap Start: 0x100000\n");
    }
    else if (command_equals("run"))
    {
        create_process_with_entry("test", test_process);
    }
    else if (command_equals("wait"))
    {
        if (process_wait(0, 500) == 0)
            terminal_write("Process 0 waiting for 500 ticks\n");
        else
            terminal_write("Process 0 unavailable\n");
    }
    else if (command_equals("kill"))
    {
        if (process_terminate(0) == 0)
            terminal_write("Process 0 terminated\n");
        else
            terminal_write("Process 0 unavailable\n");
    }
    else if (command_equals("ticks"))
    {
        terminal_write("Ticks: ");
        write_decimal(timer_ticks());
        terminal_write("\n");
    }
    else if (command_length > 0)
    {
        terminal_write("Unknown command\n");
    }

    terminal_write("> ");

    command_length = 0;
}

void shell_start()
{
    terminal_write("\nMini Kernel Shell\n");
    terminal_write("================\n");
    terminal_write("Type help for commands\n");
    terminal_write("> ");
    enable_interrupts();

    while (1)
    {
        unsigned char scancode = keyboard_read();

        /* Ignore key release */
        if (scancode & 0x80)
            continue;

        /* ENTER */
        if (scancode == 0x1C)
        {
            execute_command();
            continue;
        }

        /* BACKSPACE */
        if (scancode == 0x0E)
        {
            if (command_length > 0)
            {
                command_length--;

                terminal_putchar('\b');
            }

            continue;
        }

        char c = scancode_to_ascii(scancode);

        if (c != 0 && command_length < 49)
        {
            command[command_length] = c;
            command_length++;

            terminal_putchar(c);
        }
    }
}