#include "paging.h"
#include "terminal.h"

typedef unsigned int uint32_t;

#define PAGE_PRESENT  0x1
#define PAGE_WRITE    0x2
#define PAGE_USER     0x4

uint32_t page_directory[1024]
    __attribute__((aligned(4096)));

uint32_t first_page_table[1024]
    __attribute__((aligned(4096)));

void paging_initialize()
{
    int i;

    /* Clear page directory */
    for (i = 0; i < 1024; i++)
    {
        page_directory[i] = 0x00000002;
    }

    /* Create identity-mapped page table */
    for (i = 0; i < 1024; i++)
    {
        first_page_table[i] =
            (i * 4096) |
            PAGE_PRESENT |
            PAGE_WRITE;
    }

    /* Put first page table into page directory */
    page_directory[0] =
        ((uint32_t)first_page_table) |
        PAGE_PRESENT |
        PAGE_WRITE;

    /* Load page directory into CR3 */
    __asm__ volatile(
        "mov %0, %%cr3"
        :
        : "r"(page_directory)
    );

    /* Enable paging */
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

    terminal_write("Paging enabled\n");
}