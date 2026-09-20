ENTRY(start)

SECTIONS
{
    . = 0x00100000;

    .text :
    {
        KEEP(*(.multiboot))
        *(.text*)
    }

    .rodata :
    {
        *(.rodata*)
        *(.eh_frame*)
    }

    .data :
    {
        *(.data*)
    }

    .bss :
    {
        *(COMMON)
        *(.bss*)
    }
}