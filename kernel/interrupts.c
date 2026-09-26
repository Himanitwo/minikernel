#include <stdint.h>

#include "interrupts.h"
#include "terminal.h"
#include "scheduler.h"

#define IDT_ENTRIES 256

#define PIC_MASTER 0x20
#define PIC_SLAVE  0xA0

#define PIC_MASTER_COMMAND 0x20
#define PIC_MASTER_DATA    0x21
#define PIC_SLAVE_COMMAND  0xA0
#define PIC_SLAVE_DATA     0xA1

#define PIC_EOI 0x20

#define KEYBOARD_DATA 0x60

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

static idt_entry_t idt[IDT_ENTRIES];
static idt_register_t idt_register;

static volatile unsigned int tick_count = 0;

static unsigned char keyboard_buffer[128];
static volatile unsigned int keyboard_head = 0;
static volatile unsigned int keyboard_tail = 0;

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static void idt_set_gate(
    int number,
    uint32_t handler,
    uint16_t selector,
    uint8_t flags)
{
    idt[number].offset_low =
        (uint16_t)(handler & 0xFFFF);

    idt[number].offset_high =
        (uint16_t)((handler >> 16) & 0xFFFF);

    idt[number].selector = selector;
    idt[number].zero = 0;
    idt[number].flags = flags;
}

/* IRQ 0-7 -> INT 0x20-0x27
 * IRQ 8-15 -> INT 0x28-0x2F
 */
static void pic_remap(void)
{
    uint8_t master_mask;
    uint8_t slave_mask;

    master_mask = inb(PIC_MASTER_DATA);
    slave_mask = inb(PIC_SLAVE_DATA);

    /* Start initialization */
    outb(PIC_MASTER_COMMAND, 0x11);
    outb(PIC_SLAVE_COMMAND, 0x11);

    /* Vector offsets */
    outb(PIC_MASTER_DATA, 0x20);
    outb(PIC_SLAVE_DATA, 0x28);

    /* Tell master/slave wiring */
    outb(PIC_MASTER_DATA, 0x04);
    outb(PIC_SLAVE_DATA, 0x02);

    /* 8086 mode */
    outb(PIC_MASTER_DATA, 0x01);
    outb(PIC_SLAVE_DATA, 0x01);

    /*
     * Enable:
     * IRQ0 = timer
     * IRQ1 = keyboard
     *
     * Keep everything else masked for now.
     */
    outb(PIC_MASTER_DATA, 0xFC);
    outb(PIC_SLAVE_DATA, 0xEF);

    (void)master_mask;
    (void)slave_mask;
}

void interrupts_initialize(void)
{
    int i;

    __asm__ volatile("cli");

    for (i = 0; i < IDT_ENTRIES; i++)
    {
        extern void isr_default(void);

        idt_set_gate(
            i,
            (uint32_t)isr_default,
            0x08,
            0x8E
        );
    }

    extern void exception_stub(void);
    extern void irq0_stub(void);
    extern void irq1_stub(void);
    extern void irq12_stub(void);

    /*
     * CPU exception handler.
     */
    idt_set_gate(
        0x00,
        (uint32_t)exception_stub,
        0x08,
        0x8E
    );

    /*
     * Double fault.
     */
    idt_set_gate(
        0x08,
        (uint32_t)exception_stub,
        0x08,
        0x8E
    );

    /*
     * Timer IRQ0 -> 0x20
     */
    idt_set_gate(
        0x20,
        (uint32_t)irq0_stub,
        0x08,
        0x8E
    );

    /*
     * Keyboard IRQ1 -> 0x21
     */
    idt_set_gate(
        0x21,
        (uint32_t)irq1_stub,
        0x08,
        0x8E
    );

    /*
     * Mouse IRQ12 -> 0x2C
     * Kept masked for now.
     */
    idt_set_gate(
        0x2C,
        (uint32_t)irq12_stub,
        0x08,
        0x8E
    );

    idt_register.limit =
        sizeof(idt) - 1;

    idt_register.base =
        (uint32_t)&idt;

    __asm__ volatile(
        "lidt %0"
        :
        : "m"(idt_register)
    );

    pic_remap();

    terminal_write("Interrupts initialized\n");
}

/* ---------------- Interrupt control ---------------- */

void enable_interrupts(void)
{
    __asm__ volatile("sti");
}

void disable_interrupts(void)
{
    __asm__ volatile("cli");
}

/* ---------------- Timer ---------------- */

void timer_interrupt_handler(void)
{
    tick_count++;

    scheduler_on_tick(tick_count);

    /* Send EOI to master PIC */
    outb(PIC_MASTER_COMMAND, PIC_EOI);
}

unsigned int timer_ticks(void)
{
    return tick_count;
}

/* ---------------- Keyboard ---------------- */

void keyboard_interrupt_handler(void)
{
    uint8_t scancode;

    scancode = inb(KEYBOARD_DATA);

    /*
     * Only store key press codes.
     */
    if ((scancode & 0x80) == 0)
    {
        unsigned int next;

        next = (keyboard_head + 1) % 128;

        if (next != keyboard_tail)
        {
            keyboard_buffer[keyboard_head] = scancode;
            keyboard_head = next;
        }
    }

    outb(PIC_MASTER_COMMAND, PIC_EOI);
}

unsigned char keyboard_read(void)
{
    unsigned char value;

    while (keyboard_head == keyboard_tail)
    {
        __asm__ volatile("hlt");
    }

    value = keyboard_buffer[keyboard_tail];

    keyboard_tail =
        (keyboard_tail + 1) % 128;

    return value;
}

/* ---------------- Mouse ---------------- */

// void mouse_interrupt_handler(void)
// {
//     /*
//      * Mouse IRQ is currently masked.
//      * Keep a safe handler for later.
//      */
//     outb(PIC_SLAVE_COMMAND, PIC_EOI);
//     outb(PIC_MASTER_COMMAND, PIC_EOI);
// }

/* ---------------- Exceptions ---------------- */

void exception_handler(void)
{
    terminal_write("\n*** CPU EXCEPTION ***\n");
    terminal_write("Kernel halted.\n");

    disable_interrupts();

    for (;;)
    {
        __asm__ volatile("hlt");
    }
}

void page_fault_handler(void)
{
    terminal_write("\n*** PAGE FAULT ***\n");
    terminal_write("Kernel halted.\n");

    disable_interrupts();

    for (;;)
    {
        __asm__ volatile("hlt");
    }
}

void interrupt_software_test_handler(void)
{
    terminal_write("Software interrupt received\n");
}