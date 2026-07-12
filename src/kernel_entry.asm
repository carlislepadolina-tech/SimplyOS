[bits 32]
global _start
extern kernel_main

_start:
    cli                         ; <-- CRITICAL: Disable all hardware interrupts immediately!
    
    ; Ensure your data registers match your GDT selector (usually 0x10)
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    call kernel_main            ; Execute C entry point
    
.halt_loop:
    hlt                         ; Fallback safety lock
    jmp .halt_loop
