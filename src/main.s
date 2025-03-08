.global _start
.type _start, function
.align 16
_start:
    mov %rsp, %rdi
    add $0x8, %rdi
    jmp main
.size _start, . - _start
