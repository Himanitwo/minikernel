#include <stdint.h>
#include "keyboard.h"

static uint8_t shift_pressed = 0;

static const char normal_map[128] =
{
    0, 27,
    '1','2','3','4','5','6','7','8','9','0',
    '-','=',
    '\b',
    '\t',
    'q','w','e','r','t','y','u','i','o','p',
    '[',']',
    '\n',
    0,
    'a','s','d','f','g','h','j','k','l',
    ';','\'','`',
    0,
    '\\',
    'z','x','c','v','b','n','m',
    ',','.','/',
    0,
    '*',
    0,
    ' ',
};

static const char shift_map[128] =
{
    0, 27,
    '!','@','#','$','%','^','&','*','(',')',
    '_','+',
    '\b',
    '\t',
    'Q','W','E','R','T','Y','U','I','O','P',
    '{','}',
    '\n',
    0,
    'A','S','D','F','G','H','J','K','L',
    ':','"','~',
    0,
    '|',
    'Z','X','C','V','B','N','M',
    '<','>','?',
    0,
    '*',
    0,
    ' ',
};

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

void keyboard_initialize(void)
{
    shift_pressed = 0;
}

char keyboard_poll_char(void)
{
    uint8_t status;
    uint8_t scancode;

    /*
     * No keyboard data.
     */
    status = inb(0x64);

    if ((status & 1) == 0)
        return 0;

    /*
     * Bit 5 = 1 means mouse data.
     * Leave it for mouse_poll().
     */
    if (status & 0x20)
        return 0;

    scancode = inb(0x60);

    /* Left/right shift pressed */
    if (scancode == 0x2A ||
        scancode == 0x36)
    {
        shift_pressed = 1;
        return 0;
    }

    /* Left/right shift released */
    if (scancode == 0xAA ||
        scancode == 0xB6)
    {
        shift_pressed = 0;
        return 0;
    }

    /* Key release */
    if (scancode & 0x80)
        return 0;

    if (scancode >= 128)
        return 0;

    if (shift_pressed)
        return shift_map[scancode];

    return normal_map[scancode];
}