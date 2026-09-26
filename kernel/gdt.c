#include "gdt.h"
#include "terminal.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef struct
{
    uint16_t limit_low;
    uint16_t base_low;

    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;

} __attribute__((packed)) gdt_entry_t;

typedef struct
{
    uint16_t limit;
    uint32_t base;

} __attribute__((packed)) gdt_register_t;

static gdt_entry_t gdt[3];

static gdt_register_t gdt_register;

static void gdt_set_entry(
    int index,
    uint32_t base,
    uint32_t limit,
    uint8_t access,
    uint8_t granularity)
{
    gdt[index].base_low =
        base & 0xFFFF;

    gdt[index].base_middle =
        (base >> 16) & 0xFF;

    gdt[index].base_high =
        (base >> 24) & 0xFF;

    gdt[index].limit_low =
        limit & 0xFFFF;

    gdt[index].granularity =
        ((limit >> 16) & 0x0F) |
        (granularity & 0xF0);

    gdt[index].access = access;
}

void gdt_initialize(void)
{
    gdt_register.limit =
        sizeof(gdt) - 1;

    gdt_register.base =
        (uint32_t)&gdt;

    /* Null */
    gdt_set_entry(
        0,
        0,
        0,
        0,
        0
    );

    /* Kernel code */
    gdt_set_entry(
        1,
        0,
        0xFFFFFFFF,
        0x9A,
        0xCF
    );

    /* Kernel data */
    gdt_set_entry(
        2,
        0,
        0xFFFFFFFF,
        0x92,
        0xCF
    );

    __asm__ volatile(
        "lgdt %0\n"
        "ljmp $0x08, $1f\n"

        "1:\n"

        "mov $0x10, %%ax\n"

        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"

        :
        : "m"(gdt_register)
        : "ax"
    );

    terminal_write(
        "GDT initialized\n"
    );
}