bits 32

section .multiboot
align 4
    dd 0x1BADB002
    dd 0
    dd -(0x1BADB002)

section .text
global start
extern kernel_main

start:
    cli

    mov esp, stack_top

    call kernel_main

hang:
    hlt
    jmp hang

section .bss
align 16

stack_bottom:
    resb 16384

stack_top:

section .note.GNU-stack noalloc noexec nowrite progbits