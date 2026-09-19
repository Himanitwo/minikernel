#include "memory.h"
#include "terminal.h"

static unsigned int heap_position = 0x100000;

void memory_initialize()
{
    terminal_write("Memory manager initialized\n");
}

void* kmalloc(unsigned int size)
{
    unsigned int address = heap_position;

    heap_position += size;

    return (void*)address;
}