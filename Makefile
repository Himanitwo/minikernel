CC = gcc
LD = ld
AS = nasm
GRUB_MKRESCUE = grub2-mkrescue
QEMU = qemu-system-i386
CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-builtin \
	-fno-unwind-tables -fno-asynchronous-unwind-tables

OBJECTS = boot.o interrupts_asm.o kernel.o terminal.o interrupts.o memory.o paging.o \
	process.o scheduler.o shell.o

.PHONY: all iso run clean

all: kernel.bin

kernel.bin: $(OBJECTS)
	$(LD) -m elf_i386 -T linker.ld -o $@ $(OBJECTS)

boot.o: boot/boot.asm
	$(AS) -f elf32 $< -o $@

interrupts_asm.o: boot/interrupts.asm
	$(AS) -f elf32 $< -o $@

kernel.o: kernel/kernel.c
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
	rm -rf iso
	mkdir -p iso/boot/grub
	cp kernel.bin iso/boot/kernel.bin
	printf 'menuentry "Mini Kernel" {\n\tmultiboot /boot/kernel.bin\n\tboot\n}\n' > iso/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o mini-kernel.iso iso

run: iso
	$(QEMU) -cdrom mini-kernel.iso

clean:
	rm -f $(OBJECTS) kernel.bin mini-kernel.iso
	rm -rf iso