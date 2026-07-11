# --- Toolchain Configurations ---
NASM = nasm
GCC  = gcc
LD   = ld
OBJCOPY = objcopy
QEMU = qemu-system-i386

# --- Compilation Flags ---
CFLAGS = -m32 -ffreestanding -fno-pie -nostdlib -fno-stack-protector -Wall -Wextra

# --- Target Build Names ---
BOOT_BIN = boot.bin
ENTRY_OBJ = kernel_entry.o
KERNEL_OBJ = kernel.o
KERNEL_ELF = kernel.elf
KERNEL_BIN = kernel.bin
OS_IMG = simplyos.img

# --- Default Build Rule ---
all: clean $(OS_IMG) run

# --- Compile Bootloader ---
$(BOOT_BIN): src/boot.asm
	$(NASM) -f bin src/boot.asm -o $(BOOT_BIN)

# --- Assemble Kernel Entry Stub ---
$(ENTRY_OBJ): src/kernel_entry.asm
	$(NASM) -f elf32 src/kernel_entry.asm -o $(ENTRY_OBJ)

# --- Compile and Link Kernel ---
$(KERNEL_BIN): src/kernel.c $(ENTRY_OBJ)
	$(GCC) $(CFLAGS) -c src/kernel.c -o $(KERNEL_OBJ)
	# IMPORTANT: Link kernel_entry.o FIRST so it physically sits at 0x1000
	$(LD) -m elf_i386 -Ttext 0x1000 $(ENTRY_OBJ) $(KERNEL_OBJ) -o $(KERNEL_ELF)
	$(OBJCOPY) -O binary $(KERNEL_ELF) $(KERNEL_BIN)

# --- Build Primary Boot Drive & Safely Inject Payloads ---
$(OS_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(OS_IMG)
	# Expand image to ensure sector 100 exists safely
	truncate -s 65536 $(OS_IMG)
	# Inject File 1 Catalog Descriptor directly into Sector 100 (Offset 51200)
	printf "secret.txt\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x20\x00\x00\x00\x65\x00\x00\x00" | dd of=$(OS_IMG) bs=1 seek=51200 conv=notrunc status=none
	# Inject File 2 Catalog Descriptor 40 bytes later inside Sector 100 (Offset 51240)
	printf "motd.sys\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x20\x00\x00\x00\x66\x00\x00\x00" | dd of=$(OS_IMG) bs=1 seek=51240 conv=notrunc status=none
	# Force secret.txt contents into Sector 101 (LBA 101 / Offset 51712)
	printf "SimplyOS Core Decryption Key: [OS_MILESTONE_2026]\n\0" | dd of=$(OS_IMG) bs=1 seek=51712 conv=notrunc status=none
	# Force motd.sys contents into Sector 102 (LBA 102 / Offset 52224)
	printf "Welcome to a truly disk-driven environment!\n\0" | dd of=$(OS_IMG) bs=1 seek=52224 conv=notrunc status=none

# --- Launch QEMU Emulator ---
run: $(OS_IMG)
	$(QEMU) -drive format=raw,file=$(OS_IMG),index=0,media=disk

# --- Clean Workspace ---
clean:
	rm -f *.bin *.o *.elf $(OS_IMG)

.PHONY: all run clean
