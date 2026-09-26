#include "interrupts.h"
#include "terminal.h"
#include "scheduler.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef struct
{
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t flags;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_register_t;

extern void isr_default();
extern void exception_stub();
extern void irq1_stub();
extern void irq0_stub();
extern void software_test_stub();

static idt_entry_t idt[256];
static idt_register_t idt_register;
static volatile uint8_t keyboard_buffer[128];
static volatile uint8_t keyboard_read_position;
static volatile uint8_t keyboard_write_position;
static volatile uint32_t ticks;

static void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static void io_wait()
{
    outb(0x80, 0);
}

static void idt_set_gate(uint8_t vector, uint32_t handler)
{
    idt[vector].offset_low = handler & 0xFFFF;
    idt[vector].selector = 0x08;
    idt[vector].zero = 0;
    idt[vector].flags = 0x8E;
    idt[vector].offset_high = (handler >> 16) & 0xFFFF;
}

static void pic_remap()
{
    outb(0x20, 0x11);
    io_wait();
    outb(0xA0, 0x11);
    io_wait();
    outb(0x21, 0x20);
    io_wait();
    outb(0xA1, 0x28);
    io_wait();
    outb(0x21, 0x04);
    io_wait();
    outb(0xA1, 0x02);
    io_wait();
    outb(0x21, 0x01);
    io_wait();
    outb(0xA1, 0x01);
    io_wait();

    /* Keep IRQ0 (timer) and IRQ1 (keyboard) enabled. */
    outb(0x21, 0xFC);
    io_wait();
    outb(0xA1, 0xFF);
    io_wait();
}

static uint8_t inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

void exception_handler()
{
    disable_interrupts();
    terminal_write("\nCPU exception\n");
    terminal_write("Kernel halted\n");

    for (;;)
        __asm__ volatile("hlt");
}

void interrupt_software_test_handler()
{
    terminal_write("Software interrupt OK\n");
}

void interrupt_software_test()
{
    __asm__ volatile("int $0x80");
}

void keyboard_interrupt_handler()
{
    uint8_t scancode;
    uint8_t next_position;

    scancode = inb(0x60);
    next_position = (keyboard_write_position + 1) & 127;

    if (next_position != keyboard_read_position)
    {
        keyboard_buffer[keyboard_write_position] = scancode;
        keyboard_write_position = next_position;
    }

    outb(0x20, 0x20);
}

void timer_interrupt_handler()
{
    ticks++;
    scheduler_on_tick(ticks);
    outb(0x20, 0x20);
}

unsigned int timer_ticks()
{
    return ticks;
}

static void timer_initialize()
{
    uint16_t divisor = 11931;

    outb(0x43, 0x36);
    io_wait();
    outb(0x40, divisor & 0xFF);
    io_wait();
    outb(0x40, (divisor >> 8) & 0xFF);
    io_wait();
}

void interrupts_initialize()
{
    int vector;

    keyboard_read_position = 0;
    keyboard_write_position = 0;
    ticks = 0;

    for (vector = 0; vector < 256; vector++)
        idt_set_gate(vector, (uint32_t)isr_default);

    for (vector = 0; vector < 32; vector++)
        idt_set_gate(vector, (uint32_t)exception_stub);

    /* Accept IRQ1 on both the legacy PIC vector and the remapped vector. */
    idt_set_gate(8, (uint32_t)irq0_stub);
    idt_set_gate(32, (uint32_t)irq0_stub);
    idt_set_gate(9, (uint32_t)irq1_stub);
    idt_set_gate(33, (uint32_t)irq1_stub);
    idt_set_gate(0x80, (uint32_t)software_test_stub);
    idt_register.limit = sizeof(idt) - 1;
    idt_register.base = (uint32_t)&idt;

    pic_remap();
    timer_initialize();

    __asm__ volatile("lidt %0" : : "m"(idt_register));

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

unsigned char keyboard_read()
{
    unsigned char scancode;

    for (;;)
    {
        disable_interrupts();

        if (keyboard_read_position != keyboard_write_position)
        {
            scancode = keyboard_buffer[keyboard_read_position];
            keyboard_read_position =
                (keyboard_read_position + 1) & 127;
            enable_interrupts();
            return scancode;
        }

        enable_interrupts();
        __asm__ volatile("hlt");
    }
}

int keyboard_try_read()
{
    unsigned char scancode;
    unsigned int flags;

    __asm__ volatile (
        "pushfl\n"
        "popl %0\n"
        "cli"
        : "=r"(flags)
        :
        : "memory"
    );

    if (keyboard_read_position == keyboard_write_position)
    {
        __asm__ volatile (
            "pushl %0\n"
            "popfl"
            :
            : "r"(flags)
            : "memory", "cc"
        );
        return -1;
    }

    scancode = keyboard_buffer[keyboard_read_position];
    keyboard_read_position =
        (keyboard_read_position + 1) & 127;

    __asm__ volatile (
        "pushl %0\n"
        "popfl"
        :
        : "r"(flags)
        : "memory", "cc"
    );

    return scancode;
}