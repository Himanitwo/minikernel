#include "terminal.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

static volatile uint16_t *const VGA_MEMORY =
    (volatile uint16_t *)0xB8000;

static int terminal_row = 0;
static int terminal_column = 0;
static uint8_t terminal_color = 0x07;

static uint16_t vga_entry(unsigned char character, uint8_t color)
{
    return ((uint16_t)color << 8) | (uint16_t)character;
}

static void terminal_clear_screen(void)
{
    int row;
    int column;

    for (row = 0; row < VGA_HEIGHT; row++)
    {
        for (column = 0; column < VGA_WIDTH; column++)
        {
            VGA_MEMORY[row * VGA_WIDTH + column] =
                vga_entry(' ', terminal_color);
        }
    }
}

void terminal_initialize(void)
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = 0x07;

    terminal_clear_screen();
}

void terminal_putchar(char character)
{
    if (character == '\n')
    {
        terminal_column = 0;
        terminal_row++;

        if (terminal_row >= VGA_HEIGHT)
        {
            terminal_row = 0;
        }

        return;
    }

    if (character == '\r')
    {
        terminal_column = 0;
        return;
    }

    if (character == '\b')
    {
        if (terminal_column > 0)
        {
            terminal_column--;

            VGA_MEMORY[
                terminal_row * VGA_WIDTH +
                terminal_column
            ] = vga_entry(' ', terminal_color);
        }

        return;
    }

    VGA_MEMORY[
        terminal_row * VGA_WIDTH +
        terminal_column
    ] = vga_entry(character, terminal_color);

    terminal_column++;

    if (terminal_column >= VGA_WIDTH)
    {
        terminal_column = 0;
        terminal_row++;

        if (terminal_row >= VGA_HEIGHT)
        {
            terminal_row = 0;
        }
    }
}

void terminal_write(const char *data)
{
    int i = 0;

    while (data[i] != '\0')
    {
        terminal_putchar(data[i]);
        i++;
    }
}