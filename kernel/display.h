#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include "multiboot.h"

void display_init_dynamic(multiboot_info_t *mb_info);

void display_clear(uint32_t color);

void display_fill_rect(
    uint32_t x,
    uint32_t y,
    uint32_t width,
    uint32_t height,
    uint32_t color
);

void display_put_pixel(
    uint32_t x,
    uint32_t y,
    uint32_t color
);

uint32_t display_get_pixel(
    uint32_t x,
    uint32_t y
);

uint32_t display_get_width(void);
uint32_t display_get_height(void);

#endif