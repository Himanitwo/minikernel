#ifndef MEMORY_H
#define MEMORY_H

void memory_initialize();

void *kmalloc(
    unsigned int size
);

unsigned int memory_used();

unsigned int memory_free();

#endif