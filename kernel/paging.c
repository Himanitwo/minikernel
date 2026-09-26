#include <stdint.h>

#include "paging.h"
#include "terminal.h"

#define PAGE_PRESENT 0x001
#define PAGE_WRITE   0x002

/*
 * First 16 MB identity mapped.
 */
#define IDENTITY_TABLE_COUNT 4

/*
 * Extra page tables for the framebuffer.
 * A 1024x768x32 framebuffer is about 3 MB,
 * so one table is normally enough.
 */
#define FRAMEBUFFER_TABLE_COUNT 4

static uint32_t page_directory[1024]
    __attribute__((aligned(4096)));

static uint32_t page_tables[IDENTITY_TABLE_COUNT][1024]
    __attribute__((aligned(4096)));

static uint32_t framebuffer_tables[FRAMEBUFFER_TABLE_COUNT][1024]
    __attribute__((aligned(4096)));


void paging_initialize(multiboot_info_t *mb_info)
{
    uint32_t i;
    uint32_t table;

    /*
     * ---------------------------------------
     * Clear page directory
     * ---------------------------------------
     */

    for (i = 0; i < 1024; i++)
        page_directory[i] = 0;


    /*
     * ---------------------------------------
     * Identity map first 16 MB
     * ---------------------------------------
     */

    for (table = 0;
         table < IDENTITY_TABLE_COUNT;
         table++)
    {
        for (i = 0; i < 1024; i++)
        {
            uint32_t address;

            address =
                (table * 0x400000) +
                (i * 0x1000);

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
     * ---------------------------------------
     * Map framebuffer
     * ---------------------------------------
     */

    if (mb_info != 0 &&
        (mb_info->flags & (1 << 12)))
    {
        uint64_t framebuffer_address;
        uint64_t framebuffer_size;
        uint64_t framebuffer_end;

        framebuffer_address =
            mb_info->framebuffer_addr;

        framebuffer_size =
            (uint64_t)mb_info->framebuffer_pitch *
            (uint64_t)mb_info->framebuffer_height;

        framebuffer_end =
            framebuffer_address +
            framebuffer_size;

        /*
         * We only support framebuffer addresses
         * inside the 32-bit address space.
         */
        if ((framebuffer_address >> 32) == 0 &&
            framebuffer_end <= 0x100000000ULL)
        {
            uint32_t start;
            uint32_t end;
            uint32_t first_directory;
            uint32_t last_directory;
            uint32_t directory_count;

            start =
                ((uint32_t)framebuffer_address) &
                0xFFFFF000;

            end =
                ((uint32_t)(framebuffer_end + 0xFFF)) &
                0xFFFFF000;

            first_directory =
                start >> 22;

            last_directory =
                (end - 1) >> 22;

            directory_count =
                last_directory -
                first_directory +
                1;


            /*
             * Only create extra tables when the
             * framebuffer is outside our first
             * 16 MB identity mapping.
             */
            if (directory_count <= FRAMEBUFFER_TABLE_COUNT)
            {
                uint32_t current_directory;

                for (current_directory = first_directory;
                     current_directory <= last_directory;
                     current_directory++)
                {
                    uint32_t table_index;
                    uint32_t page;

                    /*
                     * Already mapped by the first
                     * 16 MB identity mapping.
                     */
                    if (current_directory <
                        IDENTITY_TABLE_COUNT)
                    {
                        continue;
                    }

                    table_index =
                        current_directory -
                        first_directory;

                    /*
                     * Clear framebuffer page table.
                     */
                    for (page = 0; page < 1024; page++)
                    {
                        framebuffer_tables[table_index][page] = 0;
                    }

                    /*
                     * Install framebuffer page table.
                     */
                    page_directory[current_directory] =
                        ((uint32_t)
                         framebuffer_tables[table_index]) |
                        PAGE_PRESENT |
                        PAGE_WRITE;
                }


                /*
                 * Fill framebuffer page tables.
                 */

                {
                    uint32_t address;

                    address = start;

                    while ((uint64_t)address < framebuffer_end)
                    {
                        uint32_t directory;
                        uint32_t table_index;
                        uint32_t page_index;

                        directory =
                            address >> 22;

                        /*
                         * First 16 MB are already mapped.
                         */
                        if (directory >= IDENTITY_TABLE_COUNT)
                        {
                            table_index =
                                directory -
                                first_directory;

                            page_index =
                                (address >> 12) &
                                0x3FF;

                            framebuffer_tables[table_index]
                                             [page_index] =
                                address |
                                PAGE_PRESENT |
                                PAGE_WRITE;
                        }

                        address += 0x1000;
                    }
                }
            }
        }
    }


    /*
     * ---------------------------------------
     * Load page directory
     * ---------------------------------------
     */

    __asm__ volatile(
        "mov %0, %%cr3"
        :
        : "r"(page_directory)
    );


    /*
     * ---------------------------------------
     * Enable paging
     * ---------------------------------------
     */

    {
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
            : "memory"
        );
    }


    terminal_write(
        "Paging enabled.\n"
    );
}