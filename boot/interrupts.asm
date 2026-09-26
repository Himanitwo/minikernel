BITS 32

section .text

extern exception_handler
extern page_fault_handler
extern keyboard_interrupt_handler
extern timer_interrupt_handler
extern mouse_interrupt_handler
extern interrupt_software_test_handler
extern scheduler_switch

global isr_default
global exception_stub
global page_fault_stub
global irq0_stub
global irq1_stub
global irq12_stub
global software_test_stub



isr_default:
    pusha
    popa
    iretd



exception_stub:
    pusha

    call exception_handler

    popa
    iretd



page_fault_stub:
    pusha

    call page_fault_handler

    popa

    ; Remove page-fault error code
    add esp, 4

    iretd


irq0_stub:
    pusha
    call timer_interrupt_handler
    popa
    iretd

irq1_stub:
    pusha
    call keyboard_interrupt_handler
    popa
    iretd

irq12_stub:
    pusha
    call mouse_interrupt_handler
    popa
    iretd



software_test_stub:
    pusha

    call interrupt_software_test_handler

    popa
    iretd