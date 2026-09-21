#include "paging.h"
#include "terminal.h"

typedef unsigned int uint32_t;

#define PAGE_PRESENT 0x001
#define PAGE_WRITE   0x002

#define PAGE_TABLE_COUNT 4

static uint32_t page_directory[1024]
    __attribute__((aligned(4096)));

static uint32_t page_tables[PAGE_TABLE_COUNT][1024]
    __attribute__((aligned(4096)));

void paging_initialize()
{
    int i;
    int table;

    /*
     * Clear page directory
     */
    for (i = 0; i < 1024; i++)
        page_directory[i] = 0x00000002;

    /*
     * Identity map first 16 MB
     */
    for (table = 0; table < PAGE_TABLE_COUNT; table++)
    {
        for (i = 0; i < 1024; i++)
        {
            uint32_t address =
                (table * 1024 * 4096) +
                (i * 4096);

            page_tables[table][i] =
                address |
                PAGE_PRESENT |
                PAGE_WRITE;
        }

        page_directory[table] =
            ((uint32_t)page_tables[table]) |
            PAGE_PRESENT |
            PAGE_WRITE;
    }

    /*
     * Load page directory
     */
    __asm__ volatile(
        "mov %0, %%cr3"
        :
        : "r"(page_directory)
    );

    /*
     * Enable paging
     */
    uint32_t cr0;

    __asm__ volatile(
        "mov %%cr0, %0"
        : "=r"(cr0)
    );

    cr0 |= 0x80000000;

    __asm__ volatile(
        "mov %0, %%cr0"
        :
        : "r"(cr0)
    );

    terminal_write("Paging enabled: 16 MB identity map\n");
}