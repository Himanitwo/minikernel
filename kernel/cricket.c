#include "cricket.h"

#include "interrupts.h"
#include "terminal.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define INNINGS_BALLS 12
#define INNINGS_WICKETS 10
#define DELIVERY_LAST_FRAME 16
#define READY_TICKS 100
#define RESULT_HOLD_FRAMES 8
#define DELIVERY_FRAME_TICKS 12

typedef enum
{
    GAME_TITLE,
    GAME_READY,
    GAME_DELIVERY,
    GAME_RESULT,
    GAME_PAUSED,
    GAME_INNINGS_OVER
} cricket_phase_t;

typedef enum
{
    SHOT_DRIVE,
    SHOT_LOFT,
    SHOT_DEFEND
} shot_t;

typedef enum
{
    DELIVERY_PACE,
    DELIVERY_SWING,
    DELIVERY_SPIN
} delivery_t;

static volatile unsigned short *const cricket_screen =
    (volatile unsigned short *)0xB8000;

static cricket_phase_t cricket_phase;
static shot_t cricket_shot;
static delivery_t cricket_delivery;
static unsigned int cricket_random_state;
static unsigned int cricket_score;
static unsigned int cricket_wickets;
static unsigned int cricket_balls;
static unsigned int cricket_last_tick;
static unsigned int cricket_phase_ticks;
static int cricket_delivery_frame;
static int cricket_aim;
static int cricket_delivery_line;
static int cricket_perfect_frame;
static int cricket_result_runs;
static int cricket_result_wicket;
static int cricket_result_frames;
static int cricket_extended_key;
static int cricket_exit_requested;
static int cricket_paused_from;
static const char *cricket_commentary;

static void cricket_start_delivery(void);

static void cricket_put(int x, int y, char character, unsigned char color)
{
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT)
        return;

    cricket_screen[y * VGA_WIDTH + x] =
        (unsigned short)character | ((unsigned short)color << 8);
}

static void cricket_text(int x, int y, const char *text, unsigned char color)
{
    while (*text != '\0' && x < VGA_WIDTH)
        cricket_put(x++, y, *text++, color);
}

static void cricket_center(int y, const char *text, unsigned char color)
{
    int length = 0;
    const char *character = text;

    while (*character++ != '\0')
        length++;

    cricket_text((VGA_WIDTH - length) / 2, y, text, color);
}

static void cricket_number(int x, int y, unsigned int value,
                           unsigned char color)
{
    char digits[10];
    int length = 0;

    do
    {
        digits[length++] = (char)('0' + value % 10);
        value /= 10;
    }
    while (value != 0 && length < (int)sizeof(digits));

    while (length > 0)
        cricket_put(x++, y, digits[--length], color);
}

static unsigned int cricket_random(void)
{
    cricket_random_state =
        cricket_random_state * 1664525u + 1013904223u;
    return cricket_random_state;
}

static void cricket_reset_innings(void)
{
    cricket_score = 0;
    cricket_wickets = 0;
    cricket_balls = 0;
    cricket_delivery_frame = 0;
    cricket_phase_ticks = 0;
    cricket_result_frames = 0;
    cricket_aim = 0;
    cricket_shot = SHOT_DRIVE;
    cricket_random_state = timer_ticks() ^ 0xC27A4D31u;

    if (cricket_random_state == 0)
        cricket_random_state = 1;

    cricket_start_delivery();
}

static void cricket_start_delivery(void)
{
    cricket_delivery = (delivery_t)(cricket_random() % 3);
    cricket_delivery_line = (int)(cricket_random() % 3) - 1;
    cricket_perfect_frame = 8 + (int)(cricket_random() % 3);
    cricket_delivery_frame = 0;
    cricket_aim = 0;
    cricket_shot = SHOT_DRIVE;
    cricket_phase_ticks = 0;
    cricket_phase = GAME_READY;
    cricket_last_tick = timer_ticks();
}

static void cricket_begin_delivery(void)
{
    cricket_delivery_frame = 0;
    cricket_phase_ticks = 0;
    cricket_phase = GAME_DELIVERY;
    cricket_last_tick = timer_ticks();
}

static void cricket_record_result(int runs, int wicket, const char *commentary)
{
    cricket_result_runs = runs;
    cricket_result_wicket = wicket;
    cricket_commentary = commentary;
    cricket_score += (unsigned int)runs;
    if (wicket)
        cricket_wickets++;

    cricket_balls++;
    cricket_phase_ticks = 0;
    cricket_result_frames = 0;
    cricket_phase = GAME_RESULT;
}

static void cricket_resolve_shot(void)
{
    int timing_error = cricket_delivery_frame - cricket_perfect_frame;
    int timing_distance = timing_error < 0 ? -timing_error : timing_error;
    int line_distance = cricket_aim - cricket_delivery_line;
    unsigned int roll = cricket_random() % 100;

    if (line_distance < 0)
        line_distance = -line_distance;

    if (cricket_shot == SHOT_DEFEND)
    {
        if (timing_distance <= 4 && line_distance <= 1 && roll < 65)
            cricket_record_result(1, 0, "SOFT HANDS - A QUICK SINGLE");
        else
            cricket_record_result(0, 0, "SOLID DEFENCE - NO RUN");
        return;
    }

    if (cricket_shot == SHOT_LOFT)
    {
        if (timing_distance <= 4 && line_distance == 0)
        {
            if (roll < 55)
                cricket_record_result(6, 0, "SIX! THAT IS OUT OF THE GROUND");
            else
                cricket_record_result(4, 0, "FOUR! CLEARS THE ROPE");
        }
        else if (timing_distance <= 6 && line_distance <= 1 && roll < 78)
        {
            cricket_record_result(4, 0, "IN THE GAP - FOUR!");
        }
        else if (timing_distance >= 8 && roll < 12)
        {
            cricket_record_result(0, 1, "SKIED HIGH... CAUGHT!");
        }
        else
        {
            cricket_record_result(0, 0, "MISCUE - THE FIELDERS CUT IT OFF");
        }
        return;
    }

    if (timing_distance <= 3 && line_distance == 0)
    {
        if (roll < 42)
            cricket_record_result(4, 0, "CRACKING DRIVE - FOUR!");
        else if (roll < 76)
            cricket_record_result(2, 0, "GREAT RUNNING - TWO");
        else
            cricket_record_result(1, 0, "PUSHED INTO THE GAP - ONE");
    }
    else if (timing_distance <= 6 && line_distance <= 1)
    {
        if (roll < 30)
            cricket_record_result(4, 0, "EDGED AWAY - FOUR!");
        else if (roll < 68)
            cricket_record_result(2, 0, "GOOD PLACEMENT - TWO");
        else
            cricket_record_result(1, 0, "QUICK SINGLE");
    }
    else if (timing_distance >= 8 && roll < 8)
    {
        cricket_record_result(0, 1, "OUTSIDE EDGE - TAKEN!");
    }
    else
    {
        cricket_record_result(0, 0, "BEATEN BY THE BOWLER");
    }
}

static const char *cricket_delivery_name(void)
{
    switch (cricket_delivery)
    {
        case DELIVERY_PACE: return "FAST PACE";
        case DELIVERY_SWING: return "LATE SWING";
        case DELIVERY_SPIN: return "TRICKY SPIN";
        default: return "DELIVERY";
    }
}

static void cricket_draw_scoreboard(void)
{
    unsigned int overs = cricket_balls / 6;
    unsigned int balls = cricket_balls % 6;

    cricket_text(2, 1, "MINI KERNEL", 0x0B);
    cricket_text(2, 2, "CRICKET  |  BATTING CHALLENGE", 0x0F);
    cricket_text(2, 4, "SCORE", 0x07);
    cricket_number(8, 4, cricket_score, 0x0E);
    cricket_put(13, 4, '/', 0x07);
    cricket_number(15, 4, cricket_wickets, 0x0C);
    cricket_text(18, 4, "OVERS", 0x07);
    cricket_number(25, 4, overs, 0x0E);
    cricket_put(26, 4, '.', 0x07);
    cricket_number(27, 4, balls, 0x0E);
    cricket_text(31, 4, "BALL", 0x07);
    cricket_number(36, 4,
                   cricket_balls < INNINGS_BALLS
                       ? cricket_balls + 1
                       : cricket_balls,
                   0x0E);
    cricket_text(38, 4, "/ 12", 0x07);
}

static void cricket_draw_field(void)
{
    int x;

    for (x = 3; x < VGA_WIDTH - 3; x++)
    {
        cricket_put(x, 7, '.', 0x02);
        cricket_put(x, 19, '.', 0x02);
    }

    cricket_text(7, 9, "\\       .        .         .       /", 0x0A);
    cricket_text(7, 17, "/       .        .         .       \\", 0x0A);
    cricket_text(18, 11, ".                         .", 0x02);
    cricket_text(18, 15, ".                         .", 0x02);

    for (x = 31; x <= 49; x++)
        cricket_put(x, 12, '=', 0x06);
    for (x = 31; x <= 49; x++)
        cricket_put(x, 14, '=', 0x06);
    for (x = 32; x < 49; x++)
        cricket_put(x, 13, ' ', 0x06);

    cricket_text(30, 10, "BOWLER", 0x07);
    cricket_text(49, 10, "BATTER", 0x0F);
    cricket_text(52, 12, "| |", 0x0E);
    cricket_text(52, 13, "| |", 0x0E);
    cricket_text(52, 14, "| |", 0x0E);
    cricket_text(28, 12, "o", 0x0C);

    cricket_text(32, 16, "LEG", 0x07);
    cricket_text(40, 16, "STRAIGHT", 0x07);
    cricket_text(50, 16, "OFF", 0x07);

    if (cricket_phase == GAME_DELIVERY)
    {
        int ball_x = 29 + cricket_delivery_frame * 2;
        int ball_y = 13;

        if (cricket_delivery_line < 0)
            ball_y--;
        else if (cricket_delivery_line > 0)
            ball_y++;

        if (cricket_delivery == DELIVERY_SWING &&
            cricket_delivery_frame > 4)
            ball_y += (cricket_delivery_line == 0) ? -1 : 0;
        else if (cricket_delivery == DELIVERY_SPIN &&
                 cricket_delivery_frame > 5)
            ball_x--;

        cricket_put(ball_x, ball_y, '*', 0x0C);
    }
    else if (cricket_phase == GAME_RESULT &&
             !cricket_result_wicket && cricket_result_runs > 0)
    {
        int ball_x = 53 + cricket_result_frames * 3;
        int ball_y = 13 - cricket_result_frames / 2;

        if (ball_x > 75)
            ball_x = 75;
        if (ball_y < 8)
            ball_y = 8;

        cricket_put(ball_x, ball_y,
                    cricket_result_runs >= 6 ? '*' : 'o',
                    cricket_result_runs >= 4 ? 0x0E : 0x0C);
    }

    if (cricket_phase == GAME_READY || cricket_phase == GAME_DELIVERY)
    {
        cricket_text(3, 21, cricket_delivery_name(), 0x0B);
        cricket_text(20, 21, "LINE:", 0x07);
        if (cricket_delivery_line < 0)
            cricket_text(26, 21, "LEG", 0x0E);
        else if (cricket_delivery_line > 0)
            cricket_text(26, 21, "OFF", 0x0E);
        else
            cricket_text(26, 21, "MIDDLE", 0x0E);

        if (cricket_phase == GAME_READY)
            cricket_text(37, 21, "GET READY...", 0x0E);

        cricket_text(3, 22, "A/D AIM  W LOFT  S DEFEND  SPACE SWING  P PAUSE", 0x07);
    }
    else if (cricket_phase == GAME_RESULT)
    {
        cricket_text(3, 21, cricket_commentary,
                     cricket_result_wicket ? 0x0C :
                     cricket_result_runs >= 4 ? 0x0E : 0x0A);
        cricket_text(3, 22, "GET READY FOR THE NEXT BALL...", 0x07);
    }
}

static void cricket_draw(void)
{
    int i;

    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        cricket_screen[i] = 0x0720;

    if (cricket_phase == GAME_TITLE)
    {
        cricket_center(2, "THE MINI KERNEL CRICKET CUP", 0x0E);
        cricket_center(4, "12 BALLS. 10 WICKETS. HOW BIG CAN YOU SCORE?", 0x0F);
        cricket_center(7, "READ THE LINE. PICK YOUR SHOT. TIME YOUR SWING.", 0x0B);
        cricket_center(10, "A / LEFT  : AIM LEG SIDE", 0x07);
        cricket_center(11, "D / RIGHT : AIM OFF SIDE", 0x07);
        cricket_center(12, "W / UP    : LOFT (MORE RISK)", 0x0D);
        cricket_center(13, "S / DOWN  : DEFEND (SAFEST)", 0x0A);
        cricket_center(14, "SPACE     : SWING WHEN BALL IS NEAR BAT", 0x0F);
        cricket_center(16, "EACH BALL TAKES ABOUT 3 SECONDS - NO RUSH", 0x0B);
        cricket_center(17, "NO SWING? IT IS A SAFE DOT BALL.", 0x07);
        cricket_center(18, "P: PAUSE     Q / ESC: EXIT", 0x07);
        cricket_center(21, "PRESS ENTER TO TAKE YOUR GUARD", 0x0A);
    }
    else
    {
        cricket_draw_scoreboard();
        cricket_draw_field();

        if (cricket_phase == GAME_PAUSED)
        {
            cricket_center(21, "INNINGS PAUSED - PRESS P TO RESUME", 0x0E);
            cricket_center(22, "Q / ESC: EXIT", 0x07);
        }
        else if (cricket_phase == GAME_INNINGS_OVER)
        {
            cricket_center(20, "INNINGS COMPLETE", 0x0E);
            cricket_center(21, "PRESS ENTER FOR A NEW INNINGS", 0x0A);
            cricket_center(22, "Q / ESC TO RETURN TO SHELL", 0x07);
        }

        if (cricket_phase == GAME_READY || cricket_phase == GAME_DELIVERY)
        {
            cricket_text(35, 18, "AIM:", 0x07);
            if (cricket_aim < 0)
                cricket_text(40, 18, "LEG", 0x0E);
            else if (cricket_aim > 0)
                cricket_text(40, 18, "OFF", 0x0E);
            else
                cricket_text(40, 18, "STRAIGHT", 0x0E);

            if (cricket_shot == SHOT_LOFT)
                cricket_text(51, 18, "LOFT", 0x0D);
            else if (cricket_shot == SHOT_DEFEND)
                cricket_text(51, 18, "DEFEND", 0x0A);
            else
                cricket_text(51, 18, "DRIVE", 0x0B);

            if (cricket_phase == GAME_READY)
            {
                cricket_center(20, "GET READY - SELECT AIM AND SHOT BEFORE THE BALL", 0x0E);
            }
            else
            {
                int frame;

                cricket_text(28, 20, "TIMING", 0x07);
                for (frame = 0; frame <= DELIVERY_LAST_FRAME; frame++)
                {
                    unsigned char color =
                        frame >= cricket_perfect_frame - 1 &&
                        frame <= cricket_perfect_frame + 1
                            ? 0x0A
                            : 0x08;

                    cricket_put(36 + frame * 2, 20,
                                frame == cricket_perfect_frame ? '|' : '-', color);
                }
                cricket_put(36 + cricket_delivery_frame * 2, 20, '>', 0x0E);
                cricket_center(23, "SWING ANY TIME; GREEN ZONE IS THE EASIEST HIT", 0x0A);
            }
        }
    }
}

static void cricket_finish_ball(void)
{
    if (cricket_balls >= INNINGS_BALLS ||
        cricket_wickets >= INNINGS_WICKETS)
    {
        cricket_phase = GAME_INNINGS_OVER;
        return;
    }

    cricket_start_delivery();
}

static void cricket_handle_key(int scancode)
{
    if (scancode == 0xE0)
    {
        cricket_extended_key = 1;
        return;
    }

    if (scancode & 0x80)
    {
        cricket_extended_key = 0;
        return;
    }

    if (cricket_extended_key)
    {
        cricket_extended_key = 0;

        if (scancode == 0x4B &&
            (cricket_phase == GAME_READY || cricket_phase == GAME_DELIVERY))
            cricket_aim = -1;
        else if (scancode == 0x4D &&
                 (cricket_phase == GAME_READY || cricket_phase == GAME_DELIVERY))
            cricket_aim = 1;
        else if (scancode == 0x48 &&
                 (cricket_phase == GAME_READY || cricket_phase == GAME_DELIVERY))
            cricket_shot = SHOT_LOFT;
        else if (scancode == 0x50 &&
                 (cricket_phase == GAME_READY || cricket_phase == GAME_DELIVERY))
            cricket_shot = SHOT_DEFEND;
        return;
    }

    switch (scancode)
    {
        case 0x01:
        case 0x10:
            cricket_exit_requested = 1;
            return;
        case 0x19:
            if (cricket_phase == GAME_PAUSED)
                cricket_phase = (cricket_phase_t)cricket_paused_from;
            else if (cricket_phase == GAME_READY ||
                     cricket_phase == GAME_DELIVERY ||
                     cricket_phase == GAME_RESULT)
            {
                cricket_paused_from = cricket_phase;
                cricket_phase = GAME_PAUSED;
            }
            cricket_last_tick = timer_ticks();
            return;
        case 0x1C:
            if (cricket_phase == GAME_TITLE ||
                cricket_phase == GAME_INNINGS_OVER)
                cricket_reset_innings();
            return;
        case 0x1E:
            if (cricket_phase == GAME_READY || cricket_phase == GAME_DELIVERY)
                cricket_aim = -1;
            return;
        case 0x20:
            if (cricket_phase == GAME_READY || cricket_phase == GAME_DELIVERY)
                cricket_aim = 1;
            return;
        case 0x11:
            if (cricket_phase == GAME_READY || cricket_phase == GAME_DELIVERY)
                cricket_shot = SHOT_LOFT;
            return;
        case 0x1F:
            if (cricket_phase == GAME_READY || cricket_phase == GAME_DELIVERY)
                cricket_shot = SHOT_DEFEND;
            return;
        case 0x39:
            if (cricket_phase == GAME_DELIVERY)
                cricket_resolve_shot();
            return;
        default:
            return;
    }
}

void game_cricket_start(void)
{
    int dirty = 1;

    cricket_phase = GAME_TITLE;
    cricket_exit_requested = 0;
    cricket_extended_key = 0;
    cricket_score = 0;
    cricket_wickets = 0;
    cricket_balls = 0;
    cricket_random_state = timer_ticks() ^ 0xC27A4D31u;
    cricket_last_tick = timer_ticks();

    terminal_initialize();
    terminal_set_cursor_visible(0);

    while (!cricket_exit_requested)
    {
        int scancode;
        unsigned int now;

        while ((scancode = keyboard_try_read()) >= 0)
        {
            cricket_handle_key(scancode);
            dirty = 1;
        }

        now = timer_ticks();
        if (cricket_phase != GAME_PAUSED &&
        (unsigned int)(now - cricket_last_tick) >=
            (cricket_phase == GAME_READY ? READY_TICKS :
             DELIVERY_FRAME_TICKS))
        {
            cricket_last_tick = now;

            if (cricket_phase == GAME_READY)
            {
                cricket_begin_delivery();
                dirty = 1;
            }
            else if (cricket_phase == GAME_DELIVERY)
            {
                cricket_delivery_frame++;
                if (cricket_delivery_frame >= DELIVERY_LAST_FRAME)
                    cricket_record_result(0, 0, "NO SWING - SAFE DOT BALL");
                dirty = 1;
            }
            else if (cricket_phase == GAME_RESULT)
            {
                cricket_phase_ticks++;
                if (cricket_phase_ticks % 3 == 0 &&
                    cricket_result_frames < 7)
                    cricket_result_frames++;

                if (cricket_phase_ticks >= RESULT_HOLD_FRAMES)
                    cricket_finish_ball();

                dirty = 1;
            }
        }

        if (dirty)
        {
            cricket_draw();
            dirty = 0;
        }

        __asm__ volatile("hlt");
    }

    terminal_set_cursor_visible(1);
    terminal_initialize();
    terminal_write("Cricket closed. Type help for commands.\n");
}
