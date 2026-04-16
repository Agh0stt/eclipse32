# Eclipse32 Operating System

A **32-bit bare-metal x86 OS** built from scratch in C and NASM assembly.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────┐
│                   BIOS Power-On                     │
└───────────────────────┬─────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│  STAGE 1 BOOTLOADER  (boot/stage1/boot.asm)         │
│  • 16-bit real mode                                 │
│  • Loads Stage 2 via INT 13h LBA extensions         │
│  • Verifies Stage 2 magic (0xEC32)                  │
└───────────────────────┬─────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│  STAGE 2 BOOTLOADER  (boot/stage2/stage2.asm)       │
│  • Enables A20 line (BIOS / KBD ctrl / fast A20)    │
│  • Detects & sets VBE 1024x768x32 framebuffer       │
│  • Builds GDT (kernel/user code+data, TSS)          │
│  • Enters 32-bit Protected Mode                     │
│  • Builds IDT (256 entries, default handlers)       │
│  • Sets up 4KB paging (identity maps 0-12MB)        │
│  • Remaps PIC (IRQ0→INT32, IRQ1→INT33)              │
│  • Initializes PS/2 keyboard                        │
│  • Reads FAT32 partition via ATA PIO                │
│  • Loads KERNEL.BIN to 0x100000                     │
│  • Passes boot_info struct to kernel                │
└───────────────────────┬─────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│  KERNEL  (kernel/)                                  │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │  arch/x86   │  │  mm/         │  │ drivers/  │  │
│  │  ─ GDT/TSS  │  │  ─ PMM       │  │  ─ VGA    │  │
│  │  ─ IDT      │  │  ─ VMM       │  │  ─ VBE    │  │
│  │  ─ PIC      │  │  ─ Heap      │  │  ─ KB     │  │
│  │  ─ PIT      │  │    kmalloc   │  │  ─ ATA    │  │
│  └─────────────┘  └──────────────┘  └───────────┘  │
│  ┌─────────────────────────────────────────────┐    │
│  │  fs/fat32/  - Full FAT32 driver             │    │
│  │  ─ mount, open, read, write, seek           │    │
│  │  ─ directory listing, stat, create          │    │
│  └─────────────────────────────────────────────┘    │
│  ┌─────────────────────────────────────────────┐    │
│  │  initramfs/ - Environment + boot sequence   │    │
│  │  ─ env_set/env_get (32 variables)           │    │
│  │  ─ Graphical boot splash progress bar       │    │
│  │  ─ String library (kstrcmp, kmemcpy, etc.)  │    │
│  └─────────────────────────────────────────────┘    │
└───────────────────────┬─────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────┐
│  ECLIPSE SHELL - ESH  (shell/shell.c)               │
│  ┌────────────────────────────────────────────┐     │
│  │  Line Editor                               │     │
│  │  ─ Insert/delete at cursor                 │     │
│  │  ─ Arrow key navigation                    │     │
│  │  ─ Home/End/Delete                         │     │
│  │  ─ Ctrl+A/E/K/L/C                         │     │
│  └────────────────────────────────────────────┘     │
│  ┌────────────────────────────────────────────┐     │
│  │  Command History (128 entries)             │     │
│  │  ─ Up/Down to navigate                     │     │
│  │  ─ No duplicate consecutive entries        │     │
│  └────────────────────────────────────────────┘     │
│  ┌────────────────────────────────────────────┐     │
│  │  Scripting Engine                          │     │
│  │  ─ .sh file execution                      │     │
│  │  ─ Variables: $VAR ${VAR}                  │     │
│  │  ─ $? last exit code                       │     │
│  │  ─ if/then/else/fi blocks                  │     │
│  │  ─ && and || operators                     │     │
│  │  ─ Pipes |                                 │     │
│  │  ─ Redirection > < >>                      │     │
│  │  ─ Nested scripts (depth limit 16)         │     │
│  └────────────────────────────────────────────┘     │
│  ┌────────────────────────────────────────────┐     │
│  │  Built-in Commands                         │     │
│  │  help  echo  cd  pwd  ls  cat  touch       │     │
│  │  mkdir rm  cp  mv  env  export  set  unset │     │
│  │  alias  unalias  history  clear  uname     │     │
│  │  uptime  free  date  sleep  source  exit   │     │
│  │  reboot  halt  true  false  test           │     │
│  └────────────────────────────────────────────┘     │
│  ┌────────────────────────────────────────────┐     │
│  │  Color Output                              │     │
│  │  ─ Eclipse32 brand palette (VBE ARGB)      │     │
│  │  ─ ANSI SGR escape codes                   │     │
│  │  ─ ls: dirs=blue, scripts=green, etc.      │     │
│  │  ─ Prompt: user@host:path$ with colors     │     │
│  │  ─ Error messages in red                   │     │
│  └────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────┘
```

---

## Directory Structure

```
Eclipse32/
├── boot/
│   ├── stage1/boot.asm      16-bit MBR, BMP splash, loads stage2
│   └── stage2/stage2.asm    System init, GDT/IDT/paging/VBE/FAT32
├── kernel/
│   ├── kmain.c              Main kernel entry, subsystem init order
│   ├── kernel.h             Types, macros, port I/O, CPU ops
│   ├── boot_info.h          boot_info_t struct (stage2 → kernel)
│   ├── linker.ld            Kernel linker script (loads at 1MB)
│   ├── arch/x86/
│   │   ├── entry.asm        ASM entry, 256 ISR stubs, isr_common_handler
│   │   ├── gdt.c/h          GDT with TSS
│   │   ├── gdt_asm.asm      gdt_flush, tss_flush
│   │   ├── idt.c/h          IDT + interrupt_dispatch C handler
│   │   ├── pic.c/h          8259A PIC remapping, mask/unmask
│   │   └── pit.c/h          8253 PIT, 1000Hz tick, sleep
│   ├── mm/
│   │   ├── pmm.c/h          Bitmap physical page allocator
│   │   ├── vmm.c/h          Page table manager, map/unmap
│   │   └── heap.c/h         Boundary-tag kmalloc/kfree/krealloc
│   ├── drivers/
│   │   ├── vga/vga.c/h      VGA 80×25 text, scrolling, printf
│   │   ├── vbe/vbe.c/h      VBE framebuffer, drawing, 8×16 font
│   │   ├── keyboard/        PS/2 scancode set 1, key event queue
│   │   └── disk/ata.c/h     ATA PIO primary+secondary buses
│   ├── fs/fat32/            Full FAT32: mount, open, read, write
│   └── initramfs/           Env vars, boot splash, string library
├── shell/
│   └── shell.c/h            Eclipse Shell (esh) - full implementation
├── tools/
│   └── mkdisk.py            FAT32 disk image builder
├── assets/
│   └── splash.bmp           (place your 320×200 BMP here)
└── Makefile
```

---

## Build Requirements

```bash
# Install cross-compiler toolchain
# Option 1: use osdev.org cross-compiler
export PATH=$PATH:/usr/local/cross/bin

# Required tools
i686-elf-gcc    # C cross compiler
i686-elf-ld     # Cross linker
nasm            # Assembler
python3         # Disk image tool
qemu-system-i386 # Emulator (optional)
```

## Building

```bash
make            # Build everything → build/eclipse32.img
make run        # Build and launch in QEMU
make run-debug  # Launch with GDB stub on :1234
make clean      # Remove build artifacts
```

---

## Splash Screen

Place a **320×200 BMP** (24bpp) at `assets/splash.bmp` before building.
The Stage 1 bootloader displays it in VGA mode 13h for 3 seconds.
Adjust `SPLASH_DELAY` in `boot/stage1/boot.asm` (in seconds).

---

## Shell Features

| Feature | Description |
|---------|-------------|
| **History** | 128-entry ring buffer, Up/Down arrows |
| **Line editing** | Full cursor movement, insert/delete anywhere |
| **Variables** | `set VAR value`, `$VAR`, `${VAR}`, `$?` |
| **Environment** | `export`, `env`, per-process inheritance |
| **Aliases** | `alias ll='ls -l'`, persists for session |
| **Scripting** | `.sh` files, if/then/else/fi, &&, \|\| |
| **Redirection** | `>`, `>>`, `<` |
| **Pipes** | `\|` command chaining |
| **Colors** | VBE ARGB palette, ANSI SGR codes |
| **Prompt** | Configurable PS1 with `\u \h \w \$` |

---

## Adding Commands

To add a new built-in command to esh, edit `shell/shell.c`:

1. Add the command name to `is_builtin()`
2. Add a handler function `builtin_mycommand()`  
3. Add a case in `run_builtin()`
4. Add a help entry in `builtin_help()`

---

## Memory Map

```
0x00000000 - 0x000004FF  BIOS IVT + BDA
0x00000500 - 0x00006FFF  Stack (stage1/2)
0x00005000 - 0x00005FFF  TSS
0x00006000 - 0x00006FFF  IDT (256 × 8 bytes)
0x00007000 - 0x00007FFF  Stage2 data
0x00007C00 - 0x00007DFF  Stage1 (MBR)
0x00008000 - 0x0000FFFF  Stage2 code
0x00010000 - 0x0001FFFF  FAT32 working buffer
0x00070000 - 0x00070FFF  Page Directory
0x00071000 - 0x00073FFF  Page Tables (0-12MB)
0x00100000 - 0x001FFFFF  Kernel image
0x00200000              Kernel stack (grows down)
0xC0000000 - 0xC03FFFFF  Kernel heap (4MB)
VBE_FB_ADDR              VBE framebuffer (identity mapped)
```

---

*Eclipse32 — because every great OS starts from zero.*
