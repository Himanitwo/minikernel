#include <stdint.h>

#include "gui.h"
#include "display.h"
#include "mouse.h"
#include "process.h"
#include "scheduler.h"
#include "terminal.h"

#include "keyboard.h"
#define SCREEN_DESKTOP 0
#define SCREEN_TERMINAL 1
#define SCREEN_FILES 2

#define CURSOR_W 16
#define CURSOR_H 16
#define TERMINAL_BUFFER_SIZE 64

static char terminal_input[TERMINAL_BUFFER_SIZE];
static int terminal_input_length = 0;
static int current_screen = SCREEN_DESKTOP;

static int cursor_x = 512;
static int cursor_y = 384;

static uint32_t cursor_background[CURSOR_W * CURSOR_H];

static uint8_t cursor_visible = 0;

static uint8_t previous_left = 0;
static int terminal_started = 0;
static int process_area_started = 0;
/* =========================================================
   FONT
   ========================================================= */

static const uint8_t font[26][7] =
{
    {14,17,17,31,17,17,17}, /* A */
    {30,17,17,30,17,17,30}, /* B */
    {14,17,16,16,16,17,14}, /* C */
    {30,17,17,17,17,17,30}, /* D */
    {31,16,16,30,16,16,31}, /* E */
    {31,16,16,30,16,16,16}, /* F */
    {14,17,16,23,17,17,14}, /* G */
    {17,17,17,31,17,17,17}, /* H */
    {31,4,4,4,4,4,31},      /* I */
    {7,2,2,2,2,18,12},      /* J */
    {17,18,20,24,20,18,17}, /* K */
    {16,16,16,16,16,16,31}, /* L */
    {17,27,21,21,17,17,17}, /* M */
    {17,25,21,19,17,17,17}, /* N */
    {14,17,17,17,17,17,14}, /* O */
    {30,17,17,30,16,16,16}, /* P */
    {14,17,17,17,21,18,13}, /* Q */
    {30,17,17,30,20,18,17}, /* R */
    {15,16,16,14,1,1,30},   /* S */
    {31,4,4,4,4,4,4},       /* T */
    {17,17,17,17,17,17,14}, /* U */
    {17,17,17,17,17,10,4},  /* V */
    {17,17,17,21,21,21,10}, /* W */
    {17,17,10,4,10,17,17},  /* X */
    {17,17,10,4,4,4,4},     /* Y */
    {31,1,2,4,8,16,31}      /* Z */
};

static void draw_char(
    int x,
    int y,
    char character,
    uint32_t color,
    int scale)
{
    int row;
    int col;

    /* Convert lowercase to uppercase for our font */
    if (character >= 'a' &&
        character <= 'z')
    {
        character =
            character - 'a' + 'A';
    }

    if (character >= 'A' &&
        character <= 'Z')
    {
        const uint8_t *glyph =
            font[character - 'A'];

        for (row = 0; row < 7; row++)
        {
            for (col = 0; col < 5; col++)
            {
                if (glyph[row] &
                    (1 << (4 - col)))
                {
                    display_fill_rect(
                        x + col * scale,
                        y + row * scale,
                        scale,
                        scale,
                        color
                    );
                }
            }
        }

        return;
    }

    /* Basic numbers */
    if (character >= '0' &&
        character <= '9')
    {
        static const uint8_t digits[10][7] =
        {
            {14,17,19,21,25,17,14},
            {4,12,4,4,4,4,14},
            {14,17,1,2,4,8,31},
            {30,1,1,14,1,1,30},
            {2,6,10,18,31,2,2},
            {31,16,16,30,1,1,30},
            {14,16,16,30,17,17,14},
            {31,1,2,4,8,8,8},
            {14,17,17,14,17,17,14},
            {14,17,17,15,1,1,14}
        };

        const uint8_t *glyph =
            digits[character - '0'];

        for (row = 0; row < 7; row++)
        {
            for (col = 0; col < 5; col++)
            {
                if (glyph[row] &
                    (1 << (4 - col)))
                {
                    display_fill_rect(
                        x + col * scale,
                        y + row * scale,
                        scale,
                        scale,
                        color
                    );
                }
            }
        }

        return;
    }

    /* Common terminal characters */

    if (character == '>')
    {
        display_fill_rect(
            x,
            y + 2 * scale,
            scale,
            scale,
            color
        );

        display_fill_rect(
            x + scale,
            y + 3 * scale,
            scale,
            scale,
            color
        );

        display_fill_rect(
            x,
            y + 4 * scale,
            scale,
            scale,
            color
        );

        return;
    }

    if (character == ':')
    {
        display_fill_rect(
            x + 2 * scale,
            y + 2 * scale,
            scale,
            scale,
            color
        );

        display_fill_rect(
            x + 2 * scale,
            y + 5 * scale,
            scale,
            scale,
            color
        );

        return;
    }

    if (character == '-')
    {
        display_fill_rect(
            x,
            y + 3 * scale,
            5 * scale,
            scale,
            color
        );

        return;
    }

    if (character == '_')
    {
        display_fill_rect(
            x,
            y + 6 * scale,
            5 * scale,
            scale,
            color
        );

        return;
    }
}

static void draw_text(
    int x,
    int y,
    const char *text,
    uint32_t color,
    int scale)
{
    int position = 0;

    while (text[position] != '\0')
    {
        if (text[position] == ' ')
        {
            x += 6 * scale;
        }
        else
        {
            draw_char(
                x,
                y,
                text[position],
                color,
                scale
            );

            x += 6 * scale;
        }

        position++;
    }
}

/* =========================================================
   CURSOR
   ========================================================= */

static void cursor_erase(void)
{
    int x;
    int y;

    if (!cursor_visible)
        return;

    for (y = 0; y < CURSOR_H; y++)
    {
        for (x = 0; x < CURSOR_W; x++)
        {
            int px = cursor_x + x;
            int py = cursor_y + y;

            if (px >= 0 &&
                py >= 0 &&
                (uint32_t)px < display_get_width() &&
                (uint32_t)py < display_get_height())
            {
                display_put_pixel(
                    px,
                    py,
                    cursor_background[
                        y * CURSOR_W + x
                    ]
                );
            }
        }
    }

    cursor_visible = 0;
}

static void cursor_draw(void)
{
    int x;
    int y;

    for (y = 0; y < CURSOR_H; y++)
    {
        for (x = 0; x < CURSOR_W; x++)
        {
            int px = cursor_x + x;
            int py = cursor_y + y;

            if (px >= 0 &&
                py >= 0 &&
                (uint32_t)px < display_get_width() &&
                (uint32_t)py < display_get_height())
            {
                cursor_background[
                    y * CURSOR_W + x
                ] = display_get_pixel(
                    px,
                    py
                );
            }
        }
    }

    /* Cursor outline */
    for (y = 0; y < 15; y++)
    {
        display_put_pixel(
            cursor_x,
            cursor_y + y,
            0x00000000
        );
    }

    for (x = 0; x < 11; x++)
    {
        display_put_pixel(
            cursor_x + x,
            cursor_y + x,
            0x00000000
        );
    }

    /* Cursor white interior */
    for (y = 0; y < 13; y++)
    {
        display_put_pixel(
            cursor_x + 1,
            cursor_y + y,
            0x00FFFFFF
        );

        if (y < 10)
        {
            display_put_pixel(
                cursor_x + y + 1,
                cursor_y + y,
                0x00FFFFFF
            );
        }
    }

    cursor_visible = 1;
}

/* =========================================================
   DESKTOP
   ========================================================= */

static void draw_desktop(void)
{
    uint32_t width =
        display_get_width();

    uint32_t height =
        display_get_height();

    display_clear(0x00101635);

    /* Background panels */
    display_fill_rect(
        0, 0,
        width,
        70,
        0x000A1025
    );

    display_fill_rect(
        0, 70,
        width,
        height - 150,
        0x00151F47
    );

    /* Decorative blocks */
    display_fill_rect(
        0, 150,
        width,
        5,
        0x007C4DFF
    );

    display_fill_rect(
        0, 155,
        width / 2,
        3,
        0x0025D9FF
    );

    /* Logo */
    display_fill_rect(
        20, 18,
        36,
        36,
        0x007C4DFF
    );

    display_fill_rect(
        28, 26,
        20,
        20,
        0x0025D9FF
    );

    draw_text(
        75,
        25,
        "MINI OS",
        0x00FFFFFF,
        2
    );

    /* Status lights */
    display_fill_rect(
        width - 110,
        24,
        14,
        14,
        0x0025FF88
    );

    display_fill_rect(
        width - 80,
        24,
        14,
        14,
        0x00FFD23F
    );

    display_fill_rect(
        width - 50,
        24,
        14,
        14,
        0x00FF536B
    );

    /* ---------------- TERMINAL ICON ---------------- */

    display_fill_rect(
        100,
        220,
        150,
        125,
        0x00242C52
    );

    display_fill_rect(
        100,
        220,
        150,
        15,
        0x007C4DFF
    );

    /* Terminal screen */
    display_fill_rect(
        115,
        245,
        120,
        70,
        0x00060912
    );

    draw_text(
        125,
        260,
        "TERM",
        0x0025FF88,
        2
    );

    draw_text(
        125,
        285,
        ">",
        0x00FFFFFF,
        2
    );

    draw_text(
        100,
        360,
        "TERMINAL",
        0x00FFFFFF,
        2
    );

    /* ---------------- FILE ICON ---------------- */

    display_fill_rect(
        330,
        220,
        150,
        125,
        0x00242C52
    );

    display_fill_rect(
        330,
        220,
        150,
        15,
        0x0025D9FF
    );

    /* Folder */
    display_fill_rect(
        355,
        255,
        100,
        60,
        0x00FFD23F
    );

    display_fill_rect(
        355,
        247,
        42,
        14,
        0x00FFD23F
    );

    draw_text(
        350,
        360,
        "FILES",
        0x00FFFFFF,
        2
    );

    /* Center information */
    display_fill_rect(
        570,
        200,
        350,
        245,
        0x00202A50
    );

    draw_text(
        600,
        230,
        "DESKTOP",
        0x007C4DFF,
        3
    );

    draw_text(
        600,
        285,
        "MINI OS",
        0x00FFFFFF,
        2
    );

    draw_text(
        600,
        325,
        "READY",
        0x0025FF88,
        2
    );

    /* Bottom dock */
    display_fill_rect(
        245,
        height - 100,
        535,
        65,
        0x000A1025
    );

    display_fill_rect(
        270,
        height - 82,
        35,
        35,
        0x007C4DFF
    );

    display_fill_rect(
        350,
        height - 82,
        35,
        35,
        0x0025D9FF
    );

    display_fill_rect(
        430,
        height - 82,
        35,
        35,
        0x00FF4D88
    );

    display_fill_rect(
        510,
        height - 82,
        35,
        35,
        0x00FFD23F
    );

    display_fill_rect(
        590,
        height - 82,
        35,
        35,
        0x0025FF88
    );

    display_fill_rect(
        670,
        height - 82,
        35,
        35,
        0x00FF7043
    );
}

/* =========================================================
   TERMINAL WINDOW
   ========================================================= */
static void terminal_add_char(char c)
{
    if (c == '\b')
    {
        if (terminal_input_length > 0)
            terminal_input_length--;

        return;
    }

    if (c == '\n')
{
    terminal_input[terminal_input_length] = '\0';

    terminal_input_length = 0;

    return;
}

    if (c >= 32 &&
    c <= 126 &&
    terminal_input_length <
        TERMINAL_BUFFER_SIZE - 1)
{
    terminal_input[
        terminal_input_length
    ] = c;

    terminal_input_length++;
}
}
static void draw_terminal(void)
{
    display_fill_rect(
        0,
        0,
        display_get_width(),
        display_get_height(),
        0x00070B16
    );

    /* Window */
    display_fill_rect(
        100,
        70,
        824,
        620,
        0x000D1424
    );

    /* Header */
    display_fill_rect(
        100,
        70,
        824,
        55,
        0x00172035
    );

    draw_text(
        125,
        90,
        "TERMINAL",
        0x0025FF88,
        2
    );

    /* Back */
    display_fill_rect(
        770,
        80,
        125,
        35,
        0x007C4DFF
    );

    draw_text(
        795,
        92,
        "BACK",
        0x00FFFFFF,
        1
    );

    /* Terminal */
    display_fill_rect(
        125,
        150,
        774,
        510,
        0x00020408
    );

    draw_text(
        150,
        180,
        "MINI OS TERMINAL",
        0x0025FF88,
        2
    );

    draw_text(
        150,
        220,
        "PROCESS MANAGER READY",
        0x0025D9FF,
        2
    );

    draw_text(
        150,
        260,
        "SCHEDULER READY",
        0x0025D9FF,
        2
    );

    draw_text(
        150,
        300,
        "SHELL READY",
        0x00FFD23F,
        2
    );

    draw_text(
        150,
        350,
        "AVAILABLE COMMANDS:",
        0x00FFFFFF,
        2
    );

    draw_text(
        170,
        395,
        "HELP",
        0x0025FF88,
        2
    );

    draw_text(
        170,
        435,
        "PS",
        0x0025FF88,
        2
    );

    draw_text(
        170,
        475,
        "MEM",
        0x0025FF88,
        2
    );

    draw_text(
        170,
        515,
        "RUN",
        0x0025FF88,
        2
    );

    draw_text(
        170,
        555,
        "WAIT",
        0x0025FF88,
        2
    );

    draw_text(
        150,
        610,
        "MINI-OS>",
        0x0025D9FF,
        2
    );
    draw_text(
    255,
    610,
    terminal_input,
    0x00FFFFFF,
    2
);
    display_fill_rect(
        255,
        607,
        10,
        20,
        0x00FFFFFF
    );
}

/* =========================================================
   FILES WINDOW
   ========================================================= */

static void draw_files(void)
{
    display_fill_rect(
        0,
        0,
        display_get_width(),
        display_get_height(),
        0x00101635
    );

    /* Window */
    display_fill_rect(
        150,
        80,
        720,
        600,
        0x001E2746
    );

    /* Header */
    display_fill_rect(
        150,
        80,
        720,
        55,
        0x00313B63
    );

    draw_text(
        175,
        100,
        "FILES",
        0x00FFFFFF,
        2
    );

    /* Back */
    display_fill_rect(
        720,
        90,
        120,
        35,
        0x0025D9FF
    );

    draw_text(
        750,
        102,
        "BACK",
        0x00000000,
        1
    );

    /* Static file rows */

    display_fill_rect(
        190,
        170,
        640,
        70,
        0x00283255
    );

    display_fill_rect(
        210,
        190,
        35,
        30,
        0x00FFD23F
    );

    draw_text(
        270,
        195,
        "KERNEL BIN",
        0x00FFFFFF,
        2
    );

    display_fill_rect(
        190,
        260,
        640,
        70,
        0x00283255
    );

    display_fill_rect(
        210,
        280,
        35,
        30,
        0x0025D9FF
    );

    draw_text(
        270,
        285,
        "README TXT",
        0x00FFFFFF,
        2
    );

    display_fill_rect(
        190,
        350,
        640,
        70,
        0x00283255
    );

    display_fill_rect(
        210,
        370,
        35,
        30,
        0x007C4DFF
    );

    draw_text(
        270,
        375,
        "CONFIG CFG",
        0x00FFFFFF,
        2
    );

    display_fill_rect(
        190,
        440,
        640,
        70,
        0x00283255
    );

    display_fill_rect(
        210,
        460,
        35,
        30,
        0x00FF4D88
    );

    draw_text(
        270,
        465,
        "APP C",
        0x00FFFFFF,
        2
    );
}

/* =========================================================
   SCREEN DRAW
   ========================================================= */
static void terminal_app_initialize(void)
{
    if (terminal_started)
        return;

    /*
     * Use the SAME process manager and scheduler
     * already implemented in process.c/scheduler.c.
     */
    process_initialize();

    scheduler_initialize();

    terminal_started = 1;
}
static void draw_screen(void)
{
    cursor_erase();

    if (current_screen == SCREEN_DESKTOP)
        draw_desktop();

    else if (current_screen == SCREEN_TERMINAL)
        draw_terminal();

    else if (current_screen == SCREEN_FILES)
        draw_files();

    cursor_draw();
}

/* =========================================================
   INITIALIZE
   ========================================================= */

void gui_initialize(void)
{
    mouse_state_t mouse;

    if (display_get_width() == 0 ||
        display_get_height() == 0)
    {
        return;
    }

    mouse = mouse_get_state();

    cursor_x = mouse.x;
    cursor_y = mouse.y;

    if (cursor_x < 0)
        cursor_x = 0;

    if (cursor_y < 0)
        cursor_y = 0;

    if ((uint32_t)cursor_x >= display_get_width())
        cursor_x = display_get_width() - 1;

    if ((uint32_t)cursor_y >= display_get_height())
        cursor_y = display_get_height() - 1;

    current_screen = SCREEN_DESKTOP;

    draw_screen();
}

/* =========================================================
   GUI LOOP
   ========================================================= */

void gui_run(void)
{
    mouse_state_t mouse;

    while (1)
    {
        /*
         * Mouse input
         */
        mouse_poll();

        mouse = mouse_get_state();

        /*
         * Keyboard input
         */
        if (current_screen == SCREEN_TERMINAL)
        {
            char key;

            key = keyboard_poll_char();

            if (key != 0)
            {
                terminal_add_char(key);

                /*
                 * Redraw only the terminal when
                 * keyboard input changes it.
                 */
                draw_screen();
            }
        }

        /*
         * Mouse movement
         */
        if (mouse.x != cursor_x ||
            mouse.y != cursor_y)
        {
            cursor_erase();

            cursor_x = mouse.x;
            cursor_y = mouse.y;

            if (cursor_x < 0)
                cursor_x = 0;

            if (cursor_y < 0)
                cursor_y = 0;

            if ((uint32_t)cursor_x >=
                display_get_width())
            {
                cursor_x =
                    display_get_width() - 1;
            }

            if ((uint32_t)cursor_y >=
                display_get_height())
            {
                cursor_y =
                    display_get_height() - 1;
            }

            cursor_draw();
        }

        /*
         * Mouse click
         */
        if (mouse.left_button &&
            !previous_left)
        {
            if (current_screen == SCREEN_DESKTOP)
            {
                if (cursor_x >= 100 &&
                    cursor_x <= 250 &&
                    cursor_y >= 220 &&
                    cursor_y <= 400)
                {
                    terminal_app_initialize();

                    current_screen =
                        SCREEN_TERMINAL;

                    draw_screen();
                }
                else if (cursor_x >= 330 &&
                         cursor_x <= 480 &&
                         cursor_y >= 220 &&
                         cursor_y <= 400)
                {
                    current_screen =
                        SCREEN_FILES;

                    draw_screen();
                }
            }
            else if (current_screen ==
                     SCREEN_TERMINAL)
            {
                if (cursor_x >= 750 &&
                    cursor_x <= 875 &&
                    cursor_y >= 100 &&
                    cursor_y <= 135)
                {
                    current_screen =
                        SCREEN_DESKTOP;

                    draw_screen();
                }
            }
            else if (current_screen ==
                     SCREEN_FILES)
            {
                if (cursor_x >= 720 &&
                    cursor_x <= 840 &&
                    cursor_y >= 90 &&
                    cursor_y <= 125)
                {
                    current_screen =
                        SCREEN_DESKTOP;

                    draw_screen();
                }
            }
        }

        previous_left =
            mouse.left_button;

        /*
         * Small non-blocking delay.
         */
        for (volatile int delay = 0;
             delay < 2000;
             delay++)
        {
            __asm__ volatile("nop");
        }
    }
}