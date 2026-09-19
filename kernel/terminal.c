#include "terminal.h"

volatile unsigned short *video_memory =
    (unsigned short*)0xB8000;

int row = 0;
int column = 0;

void terminal_initialize()
{
    row = 0;
    column = 0;
}

void terminal_putchar(char c)
{
    if (c == '\b')
{
    if (column > 0)
    {
        column--;

        video_memory[row * 80 + column] =
            (unsigned short)' ' | (unsigned short)0x0700;
    }

    return;
}
    if (c == '\n')
    {
        row++;
        column = 0;
        return;
    }

    video_memory[row * 80 + column] =
        (unsigned short)c | 0x0700;

    column++;

    if (column >= 80)
    {
        column = 0;
        row++;
    }
}

void terminal_write(const char *text)
{
    while (*text)
    {
        terminal_putchar(*text);
        text++;
    }
}