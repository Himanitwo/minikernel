#include "shell.h"

#include "../terminal.h"
#include "../interrupts.h"
#include "../process.h"
#include "../memory.h"
#include "../terminal.h"
static char command[50];
static int command_length = 0;

/*
 * Test process.
 */
static void test_process(void)
{
    volatile unsigned int counter = 0;

    while (1)
    {
        counter++;

        /*
         * Prevent this loop from
         * being optimized away.
         */
        if (counter == 0xFFFFFFFF)
            counter = 0;
    }
}

/*
 * Convert process state
 * into printable text.
 */
static const char *process_state_name(ProcessState state)
{
    switch (state)
    {
        case READY:
            return "READY";

        case RUNNING:
            return "RUNNING";

        case WAITING:
            return "WAITING";

        case TERMINATED:
            return "TERMINATED";

        default:
            return "UNKNOWN";
    }
}

/*
 * Print decimal integer.
 */
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
        digits[length++] = '0' + (value % 10);
        value /= 10;
    }

    while (length > 0)
    {
        terminal_putchar(digits[--length]);
    }
}

/*
 * Print hexadecimal.
 */
static void write_hex(unsigned int value)
{
    const char *hex = "0123456789ABCDEF";
    int shift;

    terminal_write("0x");

    for (shift = 28; shift >= 0; shift -= 4)
    {
        terminal_putchar(hex[(value >> shift) & 0xF]);
    }
}

/*
 * Keyboard scancode
 * to ASCII.
 */
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

        case 0x39:
            return ' ';

        default:
            return 0;
    }
}

/*
 * Compare strings.
 */
static int string_equals(const char *a, const char *b)
{
    int i = 0;

    while (a[i] != '\0' || b[i] != '\0')
    {
        if (a[i] != b[i])
            return 0;

        i++;
    }

    return 1;
}

/*
 * Convert argument into PID.
 */
static int parse_pid(const char *text)
{
    int value = 0;
    int i = 0;

    if (text[0] == '\0')
        return -1;

    while (text[i] != '\0')
    {
        if (text[i] < '0' || text[i] > '9')
        {
            return -1;
        }

        value = value * 10 + (text[i] - '0');
        i++;
    }

    if (value < 0 || value >= MAX_PROCESSES)
    {
        return -1;
    }

    return value;
}

/*
 * Split command:
 *   run 3
 * into:
 *   name = run
 *   arg  = 3
 */
static void parse_command(char *name, char *argument)
{
    int i = 0;
    int j = 0;

    /* Skip spaces */
    while (command[i] == ' ')
    {
        i++;
    }

    /* Read command name */
    while (command[i] != '\0' && command[i] != ' ')
    {
        name[j++] = command[i++];
    }

    name[j] = '\0';

    /* Skip spaces */
    while (command[i] == ' ')
    {
        i++;
    }

    /* Read argument */
    j = 0;

    while (command[i] != '\0' && command[i] != ' ')
    {
        argument[j++] = command[i++];
    }

    argument[j] = '\0';
}

/*
 * Execute shell command.
 */
static void execute_command(void)
{
    char command_name[20];
    char argument[20];
    int pid;
    int i;

    command[command_length] = '\0';

    terminal_write("\n");

    parse_command(command_name, argument);

    /*
     * HELP
     */
    if (string_equals(command_name, "help"))
    {
        terminal_write("Available commands:\n");
        terminal_write("help       - Show commands\n");
        terminal_write("clear      - Clear screen\n");
        terminal_write("ps         - Show processes\n");
        terminal_write("mem        - Show memory\n");
        terminal_write("info       - Kernel information\n");
        terminal_write("run <pid>  - Create process\n");
        terminal_write("wait <pid> - Wait 500 ticks\n");
        terminal_write("kill <pid> - Terminate process\n");
        terminal_write("ticks      - Show timer ticks\n");
    }

    /*
     * CLEAR
     */
    else if (string_equals(command_name, "clear"))
    {
        terminal_initialize();
    }

    /*
     * PS
     */
    else if (string_equals(command_name, "ps"))
    {
        terminal_write("PID  STATE       NAME\n");

        for (i = 0; i < MAX_PROCESSES; i++)
        {
            if (processes[i].state != TERMINATED)
            {
                write_decimal(processes[i].pid);
                terminal_write("    ");
                terminal_write(process_state_name(processes[i].state));
                terminal_write("      ");
                terminal_write(processes[i].name);
                terminal_write("\n");
            }
        }
    }

    /*
     * MEMORY
     */
    else if (string_equals(command_name, "mem"))
    {
        terminal_write("Memory Manager\n");
        terminal_write("Heap Start: ");
        write_hex(0x00500000);
        terminal_write("\n");

        terminal_write("Heap Limit: ");
        write_hex(0x00F00000);
        terminal_write("\n");

        terminal_write("Used: ");
        write_decimal(memory_used());
        terminal_write(" bytes\n");

        terminal_write("Free: ");
        write_decimal(memory_free());
        terminal_write(" bytes\n");
    }

    /*
     * INFO
     */
    else if (string_equals(command_name, "info"))
    {
        terminal_write("MINI KERNEL INFORMATION\n");
        terminal_write("Architecture: x86 32-bit\n");
        terminal_write("Bootloader: GRUB Multiboot\n");
        terminal_write("GDT: Enabled\n");
        terminal_write("IDT/PIC: Enabled\n");
        terminal_write("Timer: PIT 100 Hz\n");
        terminal_write("Keyboard: PS/2 IRQ1\n");
        terminal_write("Processes: 10 maximum\n");
        terminal_write("Scheduler: Round Robin\n");
    }

    /*
     * RUN PID
     */
    else if (string_equals(command_name, "run"))
    {
        pid = parse_pid(argument);

        if (pid < 0)
        {
            terminal_write("Usage: run <pid>\n");
        }
        else if (processes[pid].state != TERMINATED)
        {
            terminal_write("PID already in use\n");
        }
        else if (create_process_with_pid(pid, "process", test_process) < 0)
        {
            terminal_write("Unable to create process\n");
        }
        else
        {
            terminal_write("Process ");
            write_decimal(pid);
            terminal_write(" created\n");
        }
    }

    /*
     * WAIT PID
     */
    else if (string_equals(command_name, "wait"))
    {
        pid = parse_pid(argument);

        if (pid < 0)
        {
            terminal_write("Usage: wait <pid>\n");
        }
        else if (process_wait(pid, 500) == 0)
        {
            terminal_write("Process ");
            write_decimal(pid);
            terminal_write(" waiting for 500 ticks\n");
        }
        else
        {
            terminal_write("Process ");
            write_decimal(pid);
            terminal_write(" unavailable\n");
        }
    }

    /*
     * KILL PID
     */
    else if (string_equals(command_name, "kill"))
    {
        pid = parse_pid(argument);

        if (pid < 0)
        {
            terminal_write("Usage: kill <pid>\n");
        }
        else if (process_terminate(pid) == 0)
        {
            terminal_write("Process ");
            write_decimal(pid);
            terminal_write(" terminated\n");
        }
        else
        {
            terminal_write("Process ");
            write_decimal(pid);
            terminal_write(" unavailable\n");
        }
    }

    /*
     * TICKS
     */
    else if (string_equals(command_name, "ticks"))
    {
        extern unsigned int timer_ticks(void);

        terminal_write("Ticks: ");
        write_decimal((int)timer_ticks());
        terminal_write("\n");
    }

    /*
     * UNKNOWN COMMAND
     */
    else if (command_length > 0)
    {
        terminal_write("Unknown command\n");
    }

    terminal_write("> ");

    command_length = 0;
}

/*
 * Start shell.
 */
void shell_start(void)
{
    terminal_write("\nMini Kernel Shell\n");
    terminal_write("================\n");
    terminal_write("Type help for commands\n");
    terminal_write("> ");

    enable_interrupts();

    while (1)
    {
        unsigned char scancode = keyboard_read();

        /* Ignore key-release codes */
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

        /* Normal character */
        {
            char c = scancode_to_ascii(scancode);

            if (c != 0 && command_length < 49)
            {
                command[command_length++] = c;
                terminal_putchar(c);
            }
        }
    }
}