#include <stdint.h>

#include "multiboot.h"
#include "terminal.h"
#include "gdt.h"
#include "memory.h"
#include "process.h"
#include "scheduler.h"
#include "interrupts.h"
#include "mouse.h"
#include "display.h"
#include "gui.h"
#include "keyboard.h"

void kernel_main(
    uint32_t magic,
    multiboot_info_t *mb_info)
{
    (void)magic;
    // terminal_initialize();

// terminal_initialize();
display_init_dynamic(mb_info);
terminal_write("[OK] Framebuffer\n");

mouse_initialize();
terminal_write("[OK] Mouse\n");

keyboard_initialize();
terminal_write("[OK] Keyboard\n");

gui_initialize();
terminal_write("[OK] GUI\n");

gui_run();

gui_run();
/* GUI test: keep interrupts disabled */
for (;;)
{
    __asm__ volatile ("nop");
}
}