CC = gcc
LD = ld
AS = nasm
GRUB_MKRESCUE = grub2-mkrescue
QEMU = qemu-system-i386

OBJECTS = boot.o kernel.o terminal.o interrupts.o memory.o paging.o \
	process.o scheduler.o shell.o

.PHONY: all iso run clean

all: kernel.bin

kernel.bin: $(OBJECTS)
	$(LD) -m elf_i386 -T linker.ld -o $@ $(OBJECTS)

boot.o: boot/boot.asm
	$(AS) -f elf32 $< -o $@

kernel.o: kernel/kernel.c
	$(CC) -m32 -ffreestanding -c $< -o $@

terminal.o: kernel/terminal.c
	$(CC) -m32 -ffreestanding -c $< -o $@

interrupts.o: kernel/interrupts.c
	$(CC) -m32 -ffreestanding -c $< -o $@

memory.o: kernel/memory.c
	$(CC) -m32 -ffreestanding -c $< -o $@

paging.o: kernel/paging.c
	$(CC) -m32 -ffreestanding -c $< -o $@

process.o: kernel/process.c
	$(CC) -m32 -ffreestanding -c $< -o $@

scheduler.o: kernel/scheduler.c
	$(CC) -m32 -ffreestanding -c $< -o $@

shell.o: shell/shell.c
	$(CC) -m32 -ffreestanding -c $< -o $@

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