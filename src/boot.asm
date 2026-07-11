[org 0x7c00]
[bits 16]

; --- 1. BIOS STACK & SEGMENT SETUP ---
xor ax, ax
mov ds, ax
mov es, ax
mov ss, ax
mov sp, 0x7C00 ; Safe real-mode stack

; --- 2. LOAD KERNEL FROM DISK ---
mov ah, 0x02      ; BIOS Read Sector function
mov al, 20        ; Read 20 sectors (10 KB of kernel space)
mov ch, 0         ; Cylinder 0
mov cl, 2         ; Start reading at Sector 2 (Sector 1 is this bootloader)
mov dh, 0         ; Head 0
mov bx, 0x1000    ; Load destination: 0x1000
int 0x13
jc disk_error     ; If read fails, halt

; --- 3. ENTER 32-BIT PROTECTED MODE ---
cli               ; Disable BIOS interrupts
lgdt [gdt_descriptor]

mov eax, cr0
or eax, 1         ; Set Protected Mode Enable bit
mov cr0, eax

jmp 0x08:init_pm  ; Far jump to clear pipeline and set CS

disk_error:
    cli
    hlt

[bits 32]
init_pm:
    ; --- 4. CONFIGURE 32-BIT SEGMENTS ---
    mov ax, 0x10  ; 0x10 is the Data Segment in our GDT
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; --- 5. JUMP TO KERNEL ENTRY STUB ---
    jmp 0x1000

; --- GLOBAL DESCRIPTOR TABLE (GDT) ---
gdt_start:
gdt_null: 
    dd 0, 0
gdt_code: 
    dw 0xFFFF, 0
    db 0, 10011010b, 11001111b, 0
gdt_data: 
    dw 0xFFFF, 0
    db 0, 10010010b, 11001111b, 0
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; --- BOOT SIGNATURE ---
times 510-($-$$) db 0
dw 0xAA55
