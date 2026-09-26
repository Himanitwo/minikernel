#include "game.h"

#include "interrupts.h"
#include "terminal.h"

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define FIELD_X 9
#define FIELD_Y 4
#define FIELD_WIDTH 62
#define FIELD_HEIGHT 17
#define BOARD_WIDTH (FIELD_WIDTH - 2)
#define BOARD_HEIGHT (FIELD_HEIGHT - 2)
#define MAX_SNAKE_LENGTH (BOARD_WIDTH * BOARD_HEIGHT)
#define MOVE_INTERVAL_TICKS 10

typedef enum
{
    DIRECTION_UP,
    DIRECTION_RIGHT,
    DIRECTION_DOWN,
    DIRECTION_LEFT
} direction_t;

typedef struct
{
    int x;
    int y;
} point_t;

static volatile unsigned short *const screen =
    (volatile unsigned short *)0xB8000;

static point_t snake[MAX_SNAKE_LENGTH];
static int snake_length;
static point_t food;
static direction_t direction;
static int score;
static int paused;
static int game_over;
static unsigned int random_state;

static void put_cell(int x, int y, char value, unsigned char color)
{
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT)
        return;

    screen[y * VGA_WIDTH + x] =
        (unsigned short)value | ((unsigned short)color << 8);
}

static void draw_text(int x, int y, const char *text, unsigned char color)
{
    while (*text != '\0' && x < VGA_WIDTH)
        put_cell(x++, y, *text++, color);
}

static void draw_centered(int y, const char *text, unsigned char color)
{
    int length = 0;
    const char *character = text;

    while (*character++ != '\0')
        length++;

    draw_text((VGA_WIDTH - length) / 2, y, text, color);
}

static void draw_number(int x, int y, unsigned int value,
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
        put_cell(x++, y, digits[--length], color);
}

static unsigned int random_next(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return random_state;
}

static int point_is_on_snake(int x, int y)
{
    int i;

    for (i = 0; i < snake_length; i++)
    {
        if (snake[i].x == x && snake[i].y == y)
            return 1;
    }

    return 0;
}

static int place_food(void)
{
    int attempts;

    for (attempts = 0; attempts < MAX_SNAKE_LENGTH; attempts++)
    {
        int x = (int)(random_next() % BOARD_WIDTH);
        int y = (int)(random_next() % BOARD_HEIGHT);

        if (!point_is_on_snake(x, y))
        {
            food.x = x;
            food.y = y;
            return 1;
        }
    }

    return 0;
}

static void reset_game(void)
{
    int i;
    unsigned int ticks = timer_ticks();

    snake_length = 4;
    score = 0;
    paused = 0;
    game_over = 0;
    direction = DIRECTION_RIGHT;

    for (i = 0; i < snake_length; i++)
    {
        snake[i].x = BOARD_WIDTH / 2 - i;
        snake[i].y = BOARD_HEIGHT / 2;
    }

    random_state = ticks ^ 0x9E3779B9u;
    if (random_state == 0)
        random_state = 1;

    (void)place_food();
}

static int direction_is_opposite(direction_t candidate,
                                 direction_t current)
{
    return ((candidate + 2) % 4) == current;
}

static void set_direction(direction_t candidate)
{
    if (!direction_is_opposite(candidate, direction))
        direction = candidate;
}

static void handle_key(int scancode, int *extended, int *exit_requested)
{
    if (*extended)
    {
        *extended = 0;

        if (scancode & 0x80)
            return;

        switch (scancode)
        {
            case 0x48: set_direction(DIRECTION_UP); return;
            case 0x50: set_direction(DIRECTION_DOWN); return;
            case 0x4B: set_direction(DIRECTION_LEFT); return;
            case 0x4D: set_direction(DIRECTION_RIGHT); return;
            default: return;
        }
    }

    if (scancode == 0xE0)
    {
        *extended = 1;
        return;
    }

    if (scancode & 0x80)
        return;

    switch (scancode)
    {
        case 0x01:
        case 0x10:
            *exit_requested = 1;
            return;
        case 0x19:
            if (!game_over)
                paused = !paused;
            return;
        case 0x1C:
            if (game_over)
                reset_game();
            return;
        case 0x11: set_direction(DIRECTION_UP); return;
        case 0x1F: set_direction(DIRECTION_DOWN); return;
        case 0x1E: set_direction(DIRECTION_LEFT); return;
        case 0x20: set_direction(DIRECTION_RIGHT); return;
        default: return;
    }
}

static void advance_snake(void)
{
    point_t next = snake[0];
    int ate_food;
    int i;

    switch (direction)
    {
        case DIRECTION_UP: next.y--; break;
        case DIRECTION_RIGHT: next.x++; break;
        case DIRECTION_DOWN: next.y++; break;
        case DIRECTION_LEFT: next.x--; break;
    }

    ate_food = next.x == food.x && next.y == food.y;

    if (next.x < 0 || next.x >= BOARD_WIDTH ||
        next.y < 0 || next.y >= BOARD_HEIGHT)
    {
        game_over = 1;
        return;
    }

    for (i = 0; i < snake_length; i++)
    {
        if (!ate_food && i == snake_length - 1)
            continue;

        if (snake[i].x == next.x && snake[i].y == next.y)
        {
            game_over = 1;
            return;
        }
    }

    if (ate_food)
    {
        if (snake_length == MAX_SNAKE_LENGTH)
        {
            game_over = 1;
            return;
        }

        snake_length++;
        score++;
    }

    for (i = snake_length - 1; i > 0; i--)
        snake[i] = snake[i - 1];

    snake[0] = next;

    if (ate_food && !place_food())
        game_over = 1;
}

static void draw_game(void)
{
    int x;
    int y;
    int i;

    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        screen[i] = 0x0720;

    draw_centered(1, "MINI KERNEL - SNAKE", 0x0B);
    draw_text(2, 2, "SCORE:", 0x0F);
    draw_number(9, 2, (unsigned int)score, 0x0E);

    for (x = 0; x < FIELD_WIDTH; x++)
    {
        put_cell(FIELD_X + x, FIELD_Y, '#', 0x08);
        put_cell(FIELD_X + x, FIELD_Y + FIELD_HEIGHT - 1, '#', 0x08);
    }

    for (y = 1; y < FIELD_HEIGHT - 1; y++)
    {
        put_cell(FIELD_X, FIELD_Y + y, '#', 0x08);
        put_cell(FIELD_X + FIELD_WIDTH - 1, FIELD_Y + y, '#', 0x08);
    }

    put_cell(FIELD_X + 1 + food.x, FIELD_Y + 1 + food.y, '*', 0x0C);

    for (i = snake_length - 1; i >= 1; i--)
        put_cell(FIELD_X + 1 + snake[i].x,
                 FIELD_Y + 1 + snake[i].y, 'o', 0x0A);

    put_cell(FIELD_X + 1 + snake[0].x,
             FIELD_Y + 1 + snake[0].y, '@', 0x0F);

    draw_centered(22, "ARROWS / WASD: MOVE   P: PAUSE   Q / ESC: EXIT", 0x07);

    if (game_over)
    {
        draw_centered(11, "GAME OVER", 0x0C);
        draw_centered(12, "PRESS ENTER TO PLAY AGAIN OR Q TO EXIT", 0x0F);
    }
    else if (paused)
    {
        draw_centered(11, "PAUSED - PRESS P TO RESUME", 0x0E);
    }
}

void game_snake_start(void)
{
    unsigned int last_move;
    int extended = 0;
    int exit_requested = 0;
    int dirty = 1;

    terminal_initialize();
    terminal_set_cursor_visible(0);
    reset_game();
    last_move = timer_ticks();

    for (;;)
    {
        int scancode;
        unsigned int now;

        while ((scancode = keyboard_try_read()) >= 0)
        {
            int was_game_over = game_over;

            handle_key(scancode, &extended, &exit_requested);
            if (was_game_over && !game_over)
                last_move = timer_ticks();
            dirty = 1;
        }

        if (exit_requested)
            break;

        now = timer_ticks();

        if (!game_over && !paused &&
            (unsigned int)(now - last_move) >= MOVE_INTERVAL_TICKS)
        {
            last_move = now;
            advance_snake();
            dirty = 1;
        }
        else if (paused)
        {
            last_move = now;
        }

        if (dirty)
        {
            draw_game();
            dirty = 0;
        }

        __asm__ volatile("hlt");
    }

    terminal_set_cursor_visible(1);
    terminal_initialize();
    terminal_write("Snake closed. Type help for commands.\n");
}

#define TETRIS_WIDTH 10
#define TETRIS_HEIGHT 18
#define TETRIS_X 35
#define TETRIS_Y 3
#define TETRIS_DROP_TICKS 35

static const unsigned short tetris_shapes[7][4] =
{
    {0x00F0, 0x4444, 0x00F0, 0x4444},
    {0x0066, 0x0066, 0x0066, 0x0066},
    {0x0072, 0x0262, 0x0270, 0x0232},
    {0x0036, 0x0462, 0x0360, 0x0231},
    {0x0063, 0x0264, 0x0630, 0x0132},
    {0x0071, 0x0226, 0x0470, 0x0322},
    {0x0074, 0x0622, 0x0170, 0x0223}
};

static const unsigned char tetris_colors[7] =
{
    0x0B, 0x0E, 0x0D, 0x0A, 0x0C, 0x09, 0x06
};

static unsigned char tetris_board[TETRIS_HEIGHT][TETRIS_WIDTH];
static unsigned int tetris_random_state;
static unsigned int tetris_score;
static unsigned int tetris_lines;
static unsigned int tetris_level;
static unsigned int tetris_drop_interval;
static int tetris_piece;
static int tetris_rotation;
static int tetris_piece_x;
static int tetris_piece_y;
static int tetris_paused;
static int tetris_game_over;

static unsigned int tetris_random(void)
{
    tetris_random_state =
        tetris_random_state * 1664525u + 1013904223u;
    return tetris_random_state;
}

static int tetris_cell_in_piece(int piece, int rotation, int x, int y)
{
    unsigned short shape;

    if (x < 0 || x >= 4 || y < 0 || y >= 4)
        return 0;

    shape = tetris_shapes[piece][rotation];
    return (shape & (1u << (y * 4 + x))) != 0;
}

static int tetris_can_place(int piece, int rotation, int origin_x, int origin_y)
{
    int x;
    int y;

    for (y = 0; y < 4; y++)
    {
        for (x = 0; x < 4; x++)
        {
            int board_x;
            int board_y;

            if (!tetris_cell_in_piece(piece, rotation, x, y))
                continue;

            board_x = origin_x + x;
            board_y = origin_y + y;

            if (board_x < 0 || board_x >= TETRIS_WIDTH ||
                board_y >= TETRIS_HEIGHT)
                return 0;

            if (board_y >= 0 && tetris_board[board_y][board_x])
                return 0;
        }
    }

    return 1;
}

static void tetris_spawn_piece(void)
{
    tetris_piece = (int)(tetris_random() % 7);
    tetris_rotation = 0;
    tetris_piece_x = (TETRIS_WIDTH - 4) / 2;
    tetris_piece_y = -1;

    if (!tetris_can_place(tetris_piece, tetris_rotation,
                          tetris_piece_x, tetris_piece_y))
        tetris_game_over = 1;
}

static void tetris_clear_lines(void)
{
    int y = TETRIS_HEIGHT - 1;
    int x;
    int full;
    unsigned int cleared = 0;

    while (y >= 0)
    {
        full = 1;
        for (x = 0; x < TETRIS_WIDTH; x++)
        {
            if (!tetris_board[y][x])
            {
                full = 0;
                break;
            }
        }

        if (!full)
        {
            y--;
            continue;
        }

        for (; y > 0; y--)
        {
            for (x = 0; x < TETRIS_WIDTH; x++)
                tetris_board[y][x] = tetris_board[y - 1][x];
        }

        for (x = 0; x < TETRIS_WIDTH; x++)
            tetris_board[0][x] = 0;

        tetris_lines++;
        cleared++;
        y = TETRIS_HEIGHT - 1;
    }

    if (cleared != 0)
    {
        static const unsigned int line_points[] = {0, 100, 300, 500, 800};

        if (cleared <= 4)
            tetris_score += line_points[cleared] * tetris_level;

        tetris_level = tetris_lines / 10 + 1;
        if (tetris_level >= 11)
            tetris_drop_interval = 5;
        else
            tetris_drop_interval =
                TETRIS_DROP_TICKS - (tetris_level - 1) * 3;
    }
}

static void tetris_lock_piece(void)
{
    int x;
    int y;
    int blocks = 0;

    for (y = 0; y < 4; y++)
    {
        for (x = 0; x < 4; x++)
        {
            int board_x;
            int board_y;

            if (!tetris_cell_in_piece(tetris_piece, tetris_rotation, x, y))
                continue;

            board_x = tetris_piece_x + x;
            board_y = tetris_piece_y + y;

            if (board_y < 0)
            {
                tetris_game_over = 1;
                continue;
            }

            tetris_board[board_y][board_x] = tetris_colors[tetris_piece];
            blocks++;
        }
    }

    if (tetris_game_over)
        return;

    tetris_score += (unsigned int)(blocks * 10);
    tetris_clear_lines();
    tetris_spawn_piece();
}

static int tetris_step_down(void)
{
    if (tetris_can_place(tetris_piece, tetris_rotation,
                         tetris_piece_x, tetris_piece_y + 1))
    {
        tetris_piece_y++;
        return 1;
    }

    tetris_lock_piece();
    return 0;
}

static void tetris_reset(void)
{
    unsigned int tick = timer_ticks();
    int y;
    int x;

    for (y = 0; y < TETRIS_HEIGHT; y++)
    {
        for (x = 0; x < TETRIS_WIDTH; x++)
            tetris_board[y][x] = 0;
    }

    tetris_random_state = tick ^ 0xA511E9B3u;
    if (tetris_random_state == 0)
        tetris_random_state = 1;

    tetris_score = 0;
    tetris_lines = 0;
    tetris_level = 1;
    tetris_drop_interval = TETRIS_DROP_TICKS;
    tetris_paused = 0;
    tetris_game_over = 0;
    tetris_spawn_piece();
}

static void tetris_try_rotate(void)
{
    int next_rotation = (tetris_rotation + 1) % 4;
    static const int kicks[] = {0, -1, 1, -2, 2};
    int i;

    for (i = 0; i < (int)(sizeof(kicks) / sizeof(kicks[0])); i++)
    {
        if (tetris_can_place(tetris_piece, next_rotation,
                             tetris_piece_x + kicks[i], tetris_piece_y))
        {
            tetris_piece_x += kicks[i];
            tetris_rotation = next_rotation;
            return;
        }
    }
}

static void tetris_handle_key(int scancode, int *extended,
                             int *exit_requested)
{
    if (scancode == 0xE0)
    {
        *extended = 1;
        return;
    }

    if (scancode & 0x80)
    {
        if (*extended)
            *extended = 0;
        return;
    }

    if (*extended)
    {
        *extended = 0;

        if (tetris_game_over || tetris_paused)
            return;

        switch (scancode)
        {
            case 0x4B:
                if (tetris_can_place(tetris_piece, tetris_rotation,
                                     tetris_piece_x - 1, tetris_piece_y))
                    tetris_piece_x--;
                return;
            case 0x4D:
                if (tetris_can_place(tetris_piece, tetris_rotation,
                                     tetris_piece_x + 1, tetris_piece_y))
                    tetris_piece_x++;
                return;
            case 0x50:
                if (tetris_step_down())
                    tetris_score++;
                return;
            case 0x48:
                tetris_try_rotate();
                return;
            default:
                return;
        }
    }

    switch (scancode)
    {
        case 0x01:
        case 0x10:
            *exit_requested = 1;
            return;
        case 0x19:
            if (!tetris_game_over)
                tetris_paused = !tetris_paused;
            return;
        case 0x1C:
            if (tetris_game_over)
                tetris_reset();
            return;
        case 0x39:
            if (!tetris_game_over && !tetris_paused)
            {
                while (tetris_can_place(tetris_piece, tetris_rotation,
                                        tetris_piece_x, tetris_piece_y + 1))
                {
                    tetris_piece_y++;
                    tetris_score += 2;
                }
                tetris_lock_piece();
            }
            return;
        case 0x1E:
            if (!tetris_game_over && !tetris_paused &&
                tetris_can_place(tetris_piece, tetris_rotation,
                                 tetris_piece_x - 1, tetris_piece_y))
                tetris_piece_x--;
            return;
        case 0x20:
            if (!tetris_game_over && !tetris_paused &&
                tetris_can_place(tetris_piece, tetris_rotation,
                                 tetris_piece_x + 1, tetris_piece_y))
                tetris_piece_x++;
            return;
        case 0x11:
            if (!tetris_game_over && !tetris_paused)
                tetris_try_rotate();
            return;
        case 0x1F:
            if (!tetris_game_over && !tetris_paused)
            {
                if (tetris_step_down())
                    tetris_score++;
            }
            return;
        default:
            return;
    }
}

static void tetris_draw(void)
{
    int i;
    int x;
    int y;

    for (i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++)
        screen[i] = 0x0720;

    draw_centered(1, "MINI KERNEL - TETRIS", 0x0B);
    draw_text(3, 4, "SCORE:", 0x0F);
    draw_number(10, 4, tetris_score, 0x0E);
    draw_text(3, 6, "LINES:", 0x0F);
    draw_number(10, 6, tetris_lines, 0x0E);
    draw_text(3, 8, "LEVEL:", 0x0F);
    draw_number(10, 8, tetris_level, 0x0E);
    draw_text(3, 11, "LEFT/RIGHT A/D", 0x07);
    draw_text(3, 12, "MOVE   UP/W ROTATE", 0x07);
    draw_text(3, 14, "DOWN/S: SOFT DROP", 0x07);
    draw_text(3, 16, "SPACE: HARD DROP", 0x07);
    draw_text(3, 18, "P: PAUSE", 0x07);
    draw_text(3, 19, "Q / ESC: EXIT", 0x07);

    for (y = 0; y <= TETRIS_HEIGHT; y++)
    {
        put_cell(TETRIS_X - 1, TETRIS_Y + y, '|', 0x08);
        put_cell(TETRIS_X + TETRIS_WIDTH * 2, TETRIS_Y + y, '|', 0x08);
    }

    for (x = 0; x < TETRIS_WIDTH * 2; x++)
        put_cell(TETRIS_X - 1 + x, TETRIS_Y + TETRIS_HEIGHT, '-', 0x08);

    for (y = 0; y < TETRIS_HEIGHT; y++)
    {
        for (x = 0; x < TETRIS_WIDTH; x++)
        {
            if (tetris_board[y][x])
            {
                put_cell(TETRIS_X + x * 2, TETRIS_Y + y,
                         '[', tetris_board[y][x]);
                put_cell(TETRIS_X + x * 2 + 1, TETRIS_Y + y,
                         ']', tetris_board[y][x]);
            }
        }
    }

    if (!tetris_game_over)
    {
        for (y = 0; y < 4; y++)
        {
            for (x = 0; x < 4; x++)
            {
                int board_x = tetris_piece_x + x;
                int board_y = tetris_piece_y + y;

                if (tetris_cell_in_piece(tetris_piece, tetris_rotation, x, y) &&
                    board_y >= 0)
                {
                    put_cell(TETRIS_X + board_x * 2, TETRIS_Y + board_y,
                             '[', tetris_colors[tetris_piece]);
                    put_cell(TETRIS_X + board_x * 2 + 1, TETRIS_Y + board_y,
                             ']', tetris_colors[tetris_piece]);
                }
            }
        }
    }

    if (tetris_game_over)
    {
        draw_centered(12, "GAME OVER", 0x0C);
        draw_centered(13, "ENTER: RESTART  Q: EXIT", 0x0F);
    }
    else if (tetris_paused)
    {
        draw_centered(12, "PAUSED - PRESS P TO RESUME", 0x0E);
    }
}

void game_tetris_start(void)
{
    unsigned int last_drop;
    int extended = 0;
    int exit_requested = 0;
    int dirty = 1;

    terminal_initialize();
    terminal_set_cursor_visible(0);
    tetris_reset();
    last_drop = timer_ticks();

    for (;;)
    {
        int scancode;
        unsigned int now;

        while ((scancode = keyboard_try_read()) >= 0)
        {
            int was_game_over = tetris_game_over;

            tetris_handle_key(scancode, &extended, &exit_requested);
            if (was_game_over && !tetris_game_over)
                last_drop = timer_ticks();
            dirty = 1;
        }

        if (exit_requested)
            break;

        now = timer_ticks();
        if (!tetris_game_over && !tetris_paused &&
            (unsigned int)(now - last_drop) >= tetris_drop_interval)
        {
            last_drop = now;
            tetris_step_down();
            dirty = 1;
        }
        else if (tetris_paused)
        {
            last_drop = now;
        }

        if (dirty)
        {
            tetris_draw();
            dirty = 0;
        }

        __asm__ volatile("hlt");
    }

    terminal_set_cursor_visible(1);
    terminal_initialize();
    terminal_write("Tetris closed. Type help for commands.\n");
}
