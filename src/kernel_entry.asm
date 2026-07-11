[bits 32]
[extern kernel_main]

section .text
global _start

_start:
    ; 1. Set up a safe, guaranteed stack pointer! 
    ; Without this, the 'call' instruction crashes the CPU.
    mov esp, 0x90000 
    mov ebp, esp
    
    ; 2. Safely call the C kernel
    call kernel_main
    
    ; 3. Catch-all hang if the kernel ever returns
    cli
.hang:
    hlt
    jmp .hang
