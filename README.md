<img width="1408" height="768" alt="logo" src="https://github.com/user-attachments/assets/8d7ba969-ccc4-41e2-89a1-61dd4892e6c1" />

# SimplyOS

A lightweight, custom 32-bit x86 hobby operating system built from scratch using NASM assembly and C. It features a fully functional CLI environment driven by hardware-level port I/O, an ATA PIO disk reader, and a custom bootloader.

## Features
- **Custom Bootloader:** 16-bit real-mode initialization that switches the CPU into 32-bit Protected Mode.
- **Bare-Metal CLI:** Implemented directly via keyboard scancode mapping and VGA text mode VRAM manipulation (`0xB8000`).
- **ATA PIO Hard Drive Driver:** Reads raw sectors straight from the virtual disk image.
- **Built-in Commands:** `help`, `clear`, `dir`, `cat`, `matrix` visual effect, `colorgreen`, `colorwhite`, `reboot`, and ACPI `shutdown`.

## How to Run

### Prerequisites
Make sure you have `nasm`, `gcc` (with 32-bit support), `make`, and `qemu-system-i386` installed.

### Compilation & Emulation
Simply clone the repo and run the automation shortcut:
```bash
make clean && make run
