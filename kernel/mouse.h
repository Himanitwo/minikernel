#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

typedef struct
{
    int x;
    int y;

    uint8_t left_button;
    uint8_t right_button;
    uint8_t middle_button;

} mouse_state_t;

void mouse_initialize(void);
void mouse_interrupt_handler(void);

void mouse_poll(void);

mouse_state_t mouse_get_state(void);

#endif