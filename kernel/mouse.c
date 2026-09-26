#include <stdint.h>

#include "mouse.h"

static mouse_state_t current_mouse =
{
    512,
    384,
    0,
    0,
    0
};

static uint8_t mouse_cycle = 0;
static int8_t mouse_byte[3];

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value), "Nd"(port)
    );
}

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

static void mouse_wait_input(void)
{
    int timeout = 100000;

    while (timeout--)
    {
        if ((inb(0x64) & 2) == 0)
            return;
    }
}

static void mouse_wait_output(void)
{
    int timeout = 100000;

    while (timeout--)
    {
        if (inb(0x64) & 1)
            return;
    }
}

static void mouse_write(uint8_t value)
{
    mouse_wait_input();

    outb(0x64, 0xD4);

    mouse_wait_input();

    outb(0x60, value);
}

static uint8_t mouse_read(void)
{
    mouse_wait_output();
    return inb(0x60);
}

void mouse_initialize(void)
{
    uint8_t status;

    /*
     * Enable PS/2 auxiliary device.
     */
    mouse_wait_input();
    outb(0x64, 0xA8);

    /*
     * Read controller command byte.
     */
    mouse_wait_input();
    outb(0x64, 0x20);

    status = mouse_read();

    /*
     * Enable IRQ12.
     */
    status |= 0x02;

    /*
     * Keep translation disabled.
     */
    status &= ~0x40;

    mouse_wait_input();
    outb(0x64, 0x60);

    mouse_wait_input();
    outb(0x60, status);

    /*
     * Tell mouse to start sending packets.
     */
    mouse_write(0xF4);

    /*
     * Consume mouse ACK.
     */
    mouse_read();

    current_mouse.x = 512;
    current_mouse.y = 384;

    mouse_cycle = 0;
}

void mouse_interrupt_handler(void)
{
    uint8_t data;

    data = inb(0x60);

    if (mouse_cycle == 0)
    {
        /*
         * First byte must have bit 3 set.
         */
        if ((data & 0x08) == 0)
            return;
    }

    mouse_byte[mouse_cycle] = (int8_t)data;

    mouse_cycle++;

    if (mouse_cycle >= 3)
    {
        int dx;
        int dy;

        dx = mouse_byte[1];
        dy = mouse_byte[2];

        current_mouse.x += dx;
        current_mouse.y -= dy;

        if (current_mouse.x < 0)
            current_mouse.x = 0;

        if (current_mouse.y < 0)
            current_mouse.y = 0;

        if (current_mouse.x > 1023)
            current_mouse.x = 1023;

        if (current_mouse.y > 767)
            current_mouse.y = 767;

        current_mouse.left_button =
            mouse_byte[0] & 1;

        current_mouse.right_button =
            (mouse_byte[0] >> 1) & 1;

        current_mouse.middle_button =
            (mouse_byte[0] >> 2) & 1;

        mouse_cycle = 0;
    }
}
void mouse_poll(void)
{
    uint8_t status;
    uint8_t data;

    status = inb(0x64);

    if ((status & 0x01) == 0)
        return;

    if ((status & 0x20) == 0)
        return;

    data = inb(0x60);

    if (mouse_cycle == 0)
    {
        if ((data & 0x08) == 0)
            return;
    }

    mouse_byte[mouse_cycle] =
        (int8_t)data;

    mouse_cycle++;

    if (mouse_cycle >= 3)
    {
        int dx;
        int dy;

        dx = mouse_byte[1];
        dy = mouse_byte[2];

        current_mouse.x += dx;
        current_mouse.y -= dy;

        if (current_mouse.x < 0)
            current_mouse.x = 0;

        if (current_mouse.y < 0)
            current_mouse.y = 0;

        if (current_mouse.x >= 1024)
            current_mouse.x = 1023;

        if (current_mouse.y >= 768)
            current_mouse.y = 767;

        current_mouse.left_button =
            mouse_byte[0] & 1;

        current_mouse.right_button =
            (mouse_byte[0] >> 1) & 1;

        current_mouse.middle_button =
            (mouse_byte[0] >> 2) & 1;

        mouse_cycle = 0;
    }
}

mouse_state_t mouse_get_state(void)
{
    return current_mouse;
}