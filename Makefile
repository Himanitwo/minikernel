# ==========================================
# Mini Operating System Kernel Makefile
# ==========================================

CC = gcc
LD = ld
ASM = nasm

CFLAGS = -m32 -ffreestanding -fno-pie \
         -fno-stack-protector -fno-builtin \
         -fno-unwind-tables \
         -fno-asynchronous-unwind-tables \
         -I. -Ikernel

LDFLAGS = -m elf_i386 -T linker.ld

KERNEL = kernel.bin
ISO = mini-kernel.iso

# ==========================================
# Object Files
# ==========================================

OBJS = \
    boot/boot.o \
    boot/interrupts.o \
    kernel/kernel.o \
    kernel/gdt.o \
    kernel/terminal.o \
    kernel/memory.o \
    kernel/paging.o \
    kernel/interrupts.o \
    kernel/mouse.o \
    kernel/process.o \
    kernel/scheduler.o \
    kernel/display.o \
    kernel/gui.o \
    kernel/keyboard.o \
    kernel/shell/shell.o

# ==========================================
# Default Target
# ==========================================

all: $(KERNEL)

# ==========================================
# Build Kernel
# ==========================================

$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $(KERNEL) $(OBJS)

# ==========================================
# C Compilation
# ==========================================

kernel/kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) -c kernel/kernel.c -o kernel/kernel.o

kernel/gdt.o: kernel/gdt.c
	$(CC) $(CFLAGS) -c kernel/gdt.c -o kernel/gdt.o

kernel/terminal.o: kernel/terminal.c
	$(CC) $(CFLAGS) -c kernel/terminal.c -o kernel/terminal.o

kernel/memory.o: kernel/memory.c
	$(CC) $(CFLAGS) -c kernel/memory.c -o kernel/memory.o

kernel/paging.o: kernel/paging.c
	$(CC) $(CFLAGS) -c kernel/paging.c -o kernel/paging.o

kernel/interrupts.o: kernel/interrupts.c
	$(CC) $(CFLAGS) -c kernel/interrupts.c -o kernel/interrupts.o

kernel/mouse.o: kernel/mouse.c
	$(CC) $(CFLAGS) -c kernel/mouse.c -o kernel/mouse.o

kernel/process.o: kernel/process.c
	$(CC) $(CFLAGS) -c kernel/process.c -o kernel/process.o

kernel/scheduler.o: kernel/scheduler.c
	$(CC) $(CFLAGS) -c kernel/scheduler.c -o kernel/scheduler.o

kernel/display.o: kernel/display.c
	$(CC) $(CFLAGS) -c kernel/display.c -o kernel/display.o

kernel/gui.o: kernel/gui.c
	$(CC) $(CFLAGS) -c kernel/gui.c -o kernel/gui.o

kernel/keyboard.o: kernel/keyboard.c
	$(CC) $(CFLAGS) -c kernel/keyboard.c -o kernel/keyboard.o
kernel/shell/shell.o: kernel/shell/shell.c
	$(CC) $(CFLAGS) -c kernel/shell/shell.c -o kernel/shell/shell.o

# ==========================================
# Assembly
# ==========================================

boot/boot.o: boot/boot.asm
	$(ASM) -f elf32 boot/boot.asm -o boot/boot.o

boot/interrupts.o: boot/interrupts.asm
	$(ASM) -f elf32 boot/interrupts.asm -o boot/interrupts.o

# ==========================================
# GRUB ISO
# ==========================================
iso: $(KERNEL)
	mkdir -p iso/boot/grub
	cp $(KERNEL) iso/boot/kernel.bin
	printf 'set timeout=0\nset default=0\nset gfxmode=1024x768x32\nset gfxpayload=keep\n\nmenuentry "Mini Kernel" {\n\tmultiboot /boot/kernel.bin\n\tboot\n}\n' > iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso

# ==========================================
# Run in QEMU
# ==========================================

run: iso
	qemu-system-i386 -cdrom $(ISO)

# ==========================================
# Clean
# ==========================================

clean:
	rm -f $(OBJS)
	rm -f $(KERNEL)
	rm -f $(ISO)
	rm -rf iso

# ==========================================
# Check Multiboot
# ==========================================

check: $(KERNEL)
	grub-file --is-x86-multiboot $(KERNEL)
	@echo "KERNEL OK"