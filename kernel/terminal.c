#include "terminal.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ATTRIBUTE 0x0700

volatile unsigned short *video_memory =
    (unsigned short*)0xB8000;

int row = 0;
int column = 0;

static void outb(unsigned short port, unsigned char value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

static void terminal_update_cursor()
{
    unsigned short position = row * VGA_WIDTH + column;

    outb(0x3D4, 0x0F);
    outb(0x3D5, position & 0xFF);
    outb(0x3D4, 0x0E);
    outb(0x3D5, (position >> 8) & 0xFF);
}

static void terminal_scroll()
{
    int current_row;
    int current_column;

    for (current_row = 1; current_row < VGA_HEIGHT; current_row++)
    {
        for (current_column = 0; current_column < VGA_WIDTH; current_column++)
        {
            video_memory[(current_row - 1) * VGA_WIDTH + current_column] =
                video_memory[current_row * VGA_WIDTH + current_column];
        }
    }

    for (current_column = 0; current_column < VGA_WIDTH; current_column++)
    {
        video_memory[(VGA_HEIGHT - 1) * VGA_WIDTH + current_column] =
            (unsigned short)' ' | VGA_ATTRIBUTE;
    }

    row = VGA_HEIGHT - 1;
    terminal_update_cursor();
}

static void terminal_keep_cursor_visible()
{
    while (row >= VGA_HEIGHT)
        terminal_scroll();
}

void terminal_initialize()
{
    int current_row;
    int current_column;

    for (current_row = 0; current_row < VGA_HEIGHT; current_row++)
    {
        for (current_column = 0; current_column < VGA_WIDTH; current_column++)
        {
            video_memory[current_row * VGA_WIDTH + current_column] =
                (unsigned short)' ' | VGA_ATTRIBUTE;
        }
    }

    row = 0;
    column = 0;
    terminal_update_cursor();
}

void terminal_putchar(char c)
{
    if (c == '\b')
    {
        if (column > 0)
        {
            column--;

            video_memory[row * VGA_WIDTH + column] =
                (unsigned short)' ' | VGA_ATTRIBUTE;
            terminal_update_cursor();
        }

        return;
    }

    if (c == '\n')
    {
        row++;
        column = 0;
        terminal_keep_cursor_visible();
        terminal_update_cursor();
        return;
    }

    video_memory[row * VGA_WIDTH + column] =
        (unsigned short)c | VGA_ATTRIBUTE;

    column++;

    if (column >= VGA_WIDTH)
    {
        column = 0;
        row++;
        terminal_keep_cursor_visible();
    }

    terminal_update_cursor();
}

void terminal_write(const char *text)
{
    while (*text)
    {
        terminal_putchar(*text);
        text++;
    }
}