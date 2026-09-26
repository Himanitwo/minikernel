#include "memory.h"
#include "terminal.h"

#define HEAP_START 0x00500000
#define HEAP_LIMIT 0x00F00000

static unsigned int heap_position =
    HEAP_START;

void memory_initialize(void)
{
    heap_position =
        HEAP_START;

    terminal_write(
        "Memory manager initialized\n"
    );

    terminal_write(
        "Heap: 0x500000 - 0xF00000\n"
    );
}

void *kmalloc(unsigned int size)
{
    unsigned int address;

    if (size == 0)
        return 0;

    size =
        (size + 15) & ~15;

    if (
        heap_position + size >
        HEAP_LIMIT
    )
    {
        return 0;
    }

    address =
        heap_position;

    heap_position += size;

    return (void *)address;
}

unsigned int memory_used(void)
{
    return heap_position -
           HEAP_START;
}

unsigned int memory_free(void)
{
    return HEAP_LIMIT -
           heap_position;
}