[BITS 32]
global irq0_handler_stub
extern timer_handler

section .text

irq0_handler_stub:
    pusha               ; Save EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    push ds
    push es
    push fs
    push gs

    ; Load Kernel Data Segment (0x10 selector)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call timer_handler  ; Execute C timer function & send EOI

    pop gs
    pop fs
    pop es
    pop ds
    popa                ; Restore CPU registers
    iret                ; Return from interrupt (Restores EFLAGS, CS, EIP)