#ifndef MEMORY_H
#define MEMORY_H

void memory_initialize(void);

void *kmalloc(
    unsigned int size
);

unsigned int memory_used(void);

unsigned int memory_free(void);

#endif