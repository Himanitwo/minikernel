bits 32

section .text

extern keyboard_interrupt_handler
extern timer_interrupt_handler
extern scheduler_switch
extern process_exit_current
extern exception_handler
extern interrupt_software_test_handler

global isr_default
global exception_stub
global irq1_stub
global irq0_stub
global process_return_trampoline
global software_test_stub

isr_default:
    iretd

exception_stub:
    cli
    call exception_handler

exception_hang:
    hlt
    jmp exception_hang

irq1_stub:
    cld
    pusha
    call keyboard_interrupt_handler
    popa
    iretd

irq0_stub:
    cld
    pusha
    push esp
    call timer_interrupt_handler
    add esp, 4
    push esp
    call scheduler_switch
    add esp, 4
    mov esp, eax
    popa
    iretd

process_return_trampoline:
    call process_exit_current

process_halt:
    hlt
    jmp process_halt

software_test_stub:
    pusha
    call interrupt_software_test_handler
    popa
    iretd

section .note.GNU-stack noalloc noexec nowrite progbits
