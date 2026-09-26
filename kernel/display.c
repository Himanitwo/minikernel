#include <stdint.h>

#include "display.h"
#include "multiboot.h"

static volatile uint8_t *framebuffer = 0;

static uint32_t screen_width = 0;
static uint32_t screen_height = 0;
static uint32_t screen_pitch = 0;
static uint8_t screen_bpp = 0;


void display_init_dynamic(multiboot_info_t *mb_info)
{
    uint64_t address;

    if (mb_info == 0)
        return;

    /* Multiboot framebuffer information available */
    if ((mb_info->flags & 0x1000) == 0)
        return;

    address = mb_info->framebuffer_addr;

    /* 32-bit kernel can only directly address below 4 GB */
    if ((address >> 32) != 0)
        return;

    framebuffer =
        (volatile uint8_t *)(uint32_t)address;

    screen_width =
        mb_info->framebuffer_width;

    screen_height =
        mb_info->framebuffer_height;

    screen_pitch =
        mb_info->framebuffer_pitch;

    screen_bpp =
        mb_info->framebuffer_bpp;

    if (screen_bpp != 32)
    {
        framebuffer = 0;
        screen_width = 0;
        screen_height = 0;
        screen_pitch = 0;
        return;
    }

    if (screen_width == 0 ||
        screen_height == 0 ||
        screen_pitch == 0)
    {
        framebuffer = 0;
        screen_width = 0;
        screen_height = 0;
        screen_pitch = 0;
    }
}


/* ==========================================
   Put one pixel
   ========================================== */

void display_put_pixel(
    uint32_t x,
    uint32_t y,
    uint32_t color)
{
    volatile uint32_t *pixel;

    if (framebuffer == 0)
        return;

    if (x >= screen_width ||
        y >= screen_height)
        return;

    pixel =
        (volatile uint32_t *)
        (framebuffer +
         y * screen_pitch +
         x * 4);

    *pixel = color;
}


/* ==========================================
   Fill rectangle
   ========================================== */

void display_fill_rect(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color)
{
    uint32_t xx;
    uint32_t yy;

    if (framebuffer == 0)
        return;

    if (x >= screen_width ||
        y >= screen_height)
        return;

    if (x + width > screen_width)
        width = screen_width - x;

    if (y + height > screen_height)
        height = screen_height - y;

    for (yy = 0; yy < height; yy++)
    {
        volatile uint32_t *row =
            (volatile uint32_t *)
            (framebuffer +
             (y + yy) * screen_pitch +
             x * 4);

        for (xx = 0; xx < width; xx++)
        {
            row[xx] = color;
        }
    }
}


/* ==========================================
   Clear screen
   ========================================== */

void display_clear(uint32_t color)
{
    display_fill_rect(
        0,
        0,
        screen_width,
        screen_height,
        color
    );
}


/* ==========================================
   Read pixel
   ========================================== */

uint32_t display_get_pixel(
    uint32_t x,
    uint32_t y)
{
    volatile uint32_t *pixel;

    if (framebuffer == 0)
        return 0;

    if (x >= screen_width ||
        y >= screen_height)
        return 0;

    pixel =
        (volatile uint32_t *)
        (framebuffer +
         y * screen_pitch +
         x * 4);

    return *pixel;
}


/* ==========================================
   Screen dimensions
   ========================================== */

uint32_t display_get_width(void)
{
    return screen_width;
}

uint32_t display_get_height(void)
{
    return screen_height;
}