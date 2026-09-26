# Mini Kernel

A simple educational 32-bit mini-kernel developed in **C and x86 Assembly**. The project demonstrates basic operating-system concepts such as booting, terminal output, memory management, paging, processes, scheduling, interrupts, and a basic shell.

## Current openSUSE Tumbleweed Build

Install the required tools:

```bash
sudo zypper refresh
sudo zypper install gcc gcc-32bit glibc-devel-32bit binutils nasm grub2 grub2-i386-pc xorriso qemu-x86 make git mtools
```

Build and run from the repository directory:

```bash
make clean
make iso
make run
```

The Makefile automatically uses whichever GRUB ISO command is installed: `grub-mkrescue` or openSUSE's `grub2-mkrescue`. `mtools` is required by `grub2-mkrescue`. Always regenerate `mini-kernel.iso` after changing kernel code or the linker script.

The linker script places the Multiboot header at the beginning of the kernel image. If GRUB reports `no multiboot header found`, rebuild with `make clean` before creating the ISO.

## Snake

Start the game by entering `snake` in the kernel shell. Steer with the arrow keys or WASD, press `P` to pause or resume, and press `Q` or Escape to return to the shell. Collect `*` to grow and increase your score; hitting a wall or yourself ends the round. Press Enter to restart after game over.

## Cricket

Run `cricket` from the shell for a 12-ball batting challenge. Each ball starts with a one-second setup period, then takes about two seconds to reach the batter. Use that setup time to read the delivery and choose your aim (A/Left or D/Right) and shot (W/Up to loft, S/Down to defend); you can still adjust during the delivery. Press Space to swing, preferably while the timing marker is in the green zone. A missed swing is always a safe dot ball. Lofting can score boundaries but carries more wicket risk; defending is safest. Press P to pause, Enter to start/restart, and Q or Escape to return to the shell.

## Tetris

Start Tetris with the `tetris` shell command. Use Left/Right or A/D to move, Up or W to rotate, Down or S to soft-drop, and Space to hard-drop. `P` pauses/resumes, while `Q` or Escape returns to the shell. Completed lines increase your score; every 10 lines raises the level and speeds up falling pieces. Press Enter to restart after game over.

## 1. Project Architecture

```text
                    MINI KERNEL
                         |
        +----------------+----------------+
        |                |                |
        v                v                v
   Boot +           Memory +         Processes +
   Interrupts        Paging          Scheduling
        |                |                |
        +----------------+----------------+
                         |
                         v
                    System Calls
                         |
                         v
                       Shell
```

## 2. Technologies Used

- C
- x86 Assembly
- NASM
- GCC
- GNU LD
- GRUB
- QEMU
- WSL2
- Ubuntu
- Git/GitHub

## 3. Requirements

### Windows

Install:

- Windows 10/11
- WSL2
- Ubuntu
- VS Code
- QEMU

The actual kernel compilation should be performed inside **WSL Ubuntu**.

---

# 4. Install WSL2

Open **PowerShell as Administrator**:

```powershell
wsl --install
```

Restart the computer if requested.

Check WSL:

```powershell
wsl --status
```

Check installed distributions:

```powershell
wsl -l -v
```

Start Ubuntu:

```powershell
wsl -d Ubuntu
```

---

# 5. Install Required Packages

Open the **Ubuntu terminal**.

Update packages:

```bash
sudo apt update
sudo apt upgrade -y
```

Install compiler and build tools:

```bash
sudo apt install build-essential gcc-multilib binutils nasm grub-pc-bin grub-common xorriso qemu-system-x86 git -y
```

Check installations:

```bash
gcc --version
nasm --version
ld --version
grub2-mkrescue --version
qemu-system-i386 --version
git --version
```

---

# 6. Create the Project

Go to the home directory:

```bash
cd ~
```

Create the project:

```bash
mkdir mini-kernel
cd mini-kernel
```

Create directories:

```bash
mkdir boot kernel shell
```

Project structure:

```text
mini-kernel/
├── boot/
│   └── boot.asm
├── kernel/
│   ├── kernel.c
│   ├── terminal.c
│   ├── terminal.h
│   ├── interrupts.c
│   ├── interrupts.h
│   ├── memory.c
│   ├── memory.h
│   ├── paging.c
│   ├── paging.h
│   ├── process.c
│   ├── process.h
│   ├── scheduler.c
│   └── scheduler.h
├── shell/
│   ├── shell.c
│   └── shell.h
├── linker.ld
├── Makefile
└── README.md
```

---

# 7. Open Project in VS Code

From Ubuntu:

```bash
cd ~/mini-kernel
code .
```

Install the **WSL extension** in VS Code if required.

The VS Code window should show:

```text
WSL: Ubuntu
```

This is important because the project is stored inside the Linux filesystem.

---

# 8. Bootloader

Create:

```text
boot/boot.asm
```

The bootloader provides the Multiboot header and transfers control to `kernel_main`.

```asm
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
```

---

# 9. Linker Script

Create:

```text
linker.ld
```

```ld
ENTRY(start)

SECTIONS
{
    . = 1M;

    .text :
    {
        *(.multiboot)
        *(.text)
    }

    .data :
    {
        *(.data)
    }

    .bss :
    {
        *(.bss)
    }
}
```

---

# 10. Compile the Kernel

From:

```bash
cd ~/mini-kernel
```

Compile the bootloader:

```bash
nasm -f elf32 boot/boot.asm -o boot.o
```

Compile kernel files:

```bash
gcc -m32 -ffreestanding -c kernel/kernel.c -o kernel.o
gcc -m32 -ffreestanding -c kernel/terminal.c -o terminal.o
gcc -m32 -ffreestanding -c kernel/interrupts.c -o interrupts.o
gcc -m32 -ffreestanding -c kernel/memory.c -o memory.o
gcc -m32 -ffreestanding -c kernel/paging.c -o paging.o
gcc -m32 -ffreestanding -c kernel/process.c -o process.o
gcc -m32 -ffreestanding -c kernel/scheduler.c -o scheduler.o
```

Compile the shell:

```bash
gcc -m32 -ffreestanding -c shell/shell.c -o shell.o
```

---

# 11. Link the Kernel

```bash
ld -m elf_i386 -T linker.ld -o kernel.bin \
boot.o kernel.o terminal.o interrupts.o memory.o \
paging.o process.o scheduler.o shell.o
```

Check:

```bash
ls -lh kernel.bin
```

You should see:

```text
kernel.bin
```

A warning such as:

```text
warning: kernel.bin has a LOAD segment with RWX permissions
```

does not prevent the current educational kernel from building.

---

# 12. Create the Bootable ISO

Create the ISO directories:

```bash
rm -rf iso
mkdir -p iso/boot/grub
```

Copy the kernel:

```bash
cp kernel.bin iso/boot/kernel.bin
```

Create GRUB configuration:

```bash
nano iso/boot/grub/grub.cfg
```

Add:

```text
menuentry "Mini Kernel" {
    multiboot /boot/kernel.bin
    boot
}
```

Save:

```text
Ctrl + O
Enter
Ctrl + X
```

Create ISO:

```bash
grub2-mkrescue -o mini-kernel.iso iso
```

Check:

```bash
ls -lh mini-kernel.iso
```

---

# 13. Run the Kernel

Run:

```bash
qemu-system-i386 -cdrom mini-kernel.iso
```

QEMU should open and display something similar to:

```text
MINI KERNEL
================
Interrupt system initialized
Memory manager initialized
Process manager initialized
Scheduler initialized
Kernel initialized successfully

Mini Kernel Shell
================
Type help for commands
>
```

---

# 14. Rebuild After Making Changes

Whenever you modify the C/Assembly files:

```bash
cd ~/mini-kernel
```

Remove old build files:

```bash
rm -f *.o kernel.bin mini-kernel.iso
```

Compile:

```bash
nasm -f elf32 boot/boot.asm -o boot.o

gcc -m32 -ffreestanding -c kernel/kernel.c -o kernel.o
gcc -m32 -ffreestanding -c kernel/terminal.c -o terminal.o
gcc -m32 -ffreestanding -c kernel/interrupts.c -o interrupts.o
gcc -m32 -ffreestanding -c kernel/memory.c -o memory.o
gcc -m32 -ffreestanding -c kernel/paging.c -o paging.o
gcc -m32 -ffreestanding -c kernel/process.c -o process.o
gcc -m32 -ffreestanding -c kernel/scheduler.c -o scheduler.o
gcc -m32 -ffreestanding -c shell/shell.c -o shell.o
```

Link:

```bash
ld -m elf_i386 -T linker.ld -o kernel.bin \
boot.o kernel.o terminal.o interrupts.o memory.o \
paging.o process.o scheduler.o shell.o
```

Create ISO:

```bash
rm -rf iso
mkdir -p iso/boot/grub

cp kernel.bin iso/boot/kernel.bin

cat > iso/boot/grub/grub.cfg <<'EOF'
menuentry "Mini Kernel" {
    multiboot /boot/kernel.bin
    boot
}
EOF

grub2-mkrescue -o mini-kernel.iso iso
```

Run:

```bash
qemu-system-i386 -cdrom mini-kernel.iso
```

---

# 15. Using the Shell

The current shell supports:

```text
help
ps
mem
run
clear
wait
ticks
kill
```

### `help`

Displays available commands:

```text
Available commands:
help  - Show commands
clear - Clear screen
ps    - Show processes
mem   - Show memory
run   - Create process
wait  - Put process 0 to sleep for 500 ticks
ticks - Show timer ticks
kill  - Terminate process 0
```

### `ps`

Displays live process information:

```text
PID  STATE     NAME
0    RUNNING   test
```

### `mem`

Displays memory manager information:

```text
Memory Manager
Heap Start: 0x100000
```

### `run`

Creates a process:

```text
Process created
```

The test process has its own kernel stack and remains runnable. Timer IRQ0 performs kernel-mode context switches between the shell and process. A process can be `READY`, `RUNNING`, `WAITING`, or `TERMINATED`.

### `wait`

Places process 0 in `WAITING` for 500 timer ticks (approximately five seconds at 100 Hz):

```text
Process 0 waiting for 500 ticks
```

The timer wakes it by changing its state back to `READY`.

### `ticks`

Displays the PIT timer tick counter:

```text
Ticks: 1234
```

### `kill`

Explicitly terminates process 0:

```text
Process 0 terminated
```

Terminated process slots can be reused by a later `run` command.

### `clear`

Clears/resets the terminal.

---

# 16. Git Setup

Initialize Git **inside Ubuntu**, not from the Windows `\\wsl.localhost` path.

```bash
cd ~/mini-kernel
git init
```

Check:

```bash
git status
```

Configure Git:

```bash
git config --global user.name "YOUR_NAME"
git config --global user.email "YOUR_EMAIL"
```

Add files:

```bash
git add .
```

Commit:

```bash
git commit -m "first commit"
```

Check commits:

```bash
git log --oneline
```

---

# 17. GitHub Setup

Create an empty repository on GitHub, for example:

```text
mini-kernel
```

Then inside Ubuntu:

```bash
cd ~/mini-kernel
```

Add remote:

```bash
git remote add origin https://github.com/YOUR_USERNAME/mini-kernel.git
```

Check:

```bash
git remote -v
```

Rename branch:

```bash
git branch -M main
```

Push:

```bash
git push -u origin main
```

For future changes:

```bash
git add .
git commit -m "update kernel"
git push
```

---

# 18. Important: Do Not Run Git From This Path

Avoid:

```text
\\wsl.localhost\Ubuntu\home\admin1\mini-kernel
```

from PowerShell when doing Git operations.

Instead:

```bash
wsl
cd ~/mini-kernel
git status
```

This avoids the:

```text
fatal: detected dubious ownership
```

problem you encountered.

---

# 19. Makefile

The repository already includes an openSUSE-compatible `Makefile`. Use:

```bash
make clean
make iso
make run
```

The Makefile assembles `boot/boot.asm` and `boot/interrupts.asm`, compiles the 32-bit freestanding C sources with kernel-safe flags, links with `linker.ld`, creates the GRUB ISO with `grub2-mkrescue`, and launches QEMU.


---

# 20. Recommended Development Cycle

Every time you change code:

```bash
cd ~/mini-kernel
make clean
make iso
make run
```

Then after testing:

```bash
git add .
git commit -m "describe your change"
git push
```

Example:

```bash
git add .
git commit -m "add process scheduler"
git push
```

## Current Project Status

| Component        | Status               |
| ---------------- | -------------------- |
| Bootloader       | ✅                   |
| GRUB boot        | ✅                   |
| VGA terminal     | ✅ Scrolling/cursor  |
| Interrupt module | ✅ IDT/PIC/IRQ1      |
| Memory manager   | 🟡 Basic             |
| Paging           | 🟡 Basic             |
| Process manager  | 🟡 Kernel-mode       |
| Scheduler        | 🟡 Timer-driven      |
| Keyboard input   | ✅ IRQ1 buffered     |
| Shell            | 🟡 Basic commands    |
| System calls     | ⏳                   |
| Real IDT         | ✅ Basic handlers    |
| Timer interrupt  | ✅ PIT IRQ0 at 100 Hz|
| User mode        | ⏳                   |

**Current limitations:** process scheduling currently runs in kernel mode only. Processes have private kernel stacks and timer-driven switching, but there are no user-mode address spaces or system calls yet. Exception diagnostics are basic, paging is not enabled from the main boot path, and `WAITING`/`TERMINATED` lifecycle behavior is intended for education and testing rather than production use.
