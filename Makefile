CC = gcc
LD = ld
AS = nasm
GRUB_MKRESCUE ?= $(shell command -v grub-mkrescue 2>/dev/null || command -v grub2-mkrescue 2>/dev/null || printf grub-mkrescue)
QEMU = qemu-system-i386

CFLAGS = -m32 -ffreestanding -fno-pie \
	-fno-stack-protector -fno-builtin \
	-fno-unwind-tables \
	-fno-asynchronous-unwind-tables

OBJECTS = boot.o \
	interrupts_asm.o \
	kernel.o \
	terminal.o \
	interrupts.o \
	memory.o \
	paging.o \
	process.o \
	scheduler.o \
	gdt.o \
	game.o \
	cricket.o \
	shell.o

.PHONY: all iso run clean

all: kernel.bin


kernel.bin: $(OBJECTS)
	$(LD) -m elf_i386 \
		-T linker.ld \
		-o $@ \
		$(OBJECTS)


boot.o: boot/boot.asm
	$(AS) -f elf32 $< -o $@


interrupts_asm.o: boot/interrupts.asm
	$(AS) -f elf32 $< -o $@


kernel.o: kernel/kernel.c
	$(CC) $(CFLAGS) -c $< -o $@


gdt.o: kernel/gdt.c
	$(CC) $(CFLAGS) -c $< -o $@


game.o: kernel/game.c
	$(CC) $(CFLAGS) -c $< -o $@

cricket.o: kernel/cricket.c
	$(CC) $(CFLAGS) -c $< -o $@


terminal.o: kernel/terminal.c
	$(CC) $(CFLAGS) -c $< -o $@


interrupts.o: kernel/interrupts.c
	$(CC) $(CFLAGS) -c $< -o $@


memory.o: kernel/memory.c
	$(CC) $(CFLAGS) -c $< -o $@


paging.o: kernel/paging.c
	$(CC) $(CFLAGS) -c $< -o $@


process.o: kernel/process.c
	$(CC) $(CFLAGS) -c $< -o $@


scheduler.o: kernel/scheduler.c
	$(CC) $(CFLAGS) -c $< -o $@


shell.o: shell/shell.c
	$(CC) $(CFLAGS) -c $< -o $@


iso: kernel.bin
	@command -v "$(GRUB_MKRESCUE)" >/dev/null 2>&1 || { \
		echo "Error: GRUB ISO tool not found. Install grub2-mkrescue or grub-mkrescue, or set GRUB_MKRESCUE."; \
		exit 127; \
	}

	rm -rf iso
	mkdir -p iso/boot/grub

	cp kernel.bin iso/boot/kernel.bin

	printf 'menuentry "Mini Kernel" {\n\tmultiboot /boot/kernel.bin\n\tboot\n}\n' \
		> iso/boot/grub/grub.cfg

	$(GRUB_MKRESCUE) \
		-o mini-kernel.iso \
		iso



run: iso
	$(QEMU) -cdrom mini-kernel.iso -boot d -no-reboot -no-shutdown -d int,cpu_reset -D qemu.log


clean:
	rm -f $(OBJECTS) \
		kernel.bin \
		mini-kernel.iso

	rm -rf iso