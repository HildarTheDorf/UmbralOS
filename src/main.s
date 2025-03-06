.global _start
.type _start, function
.align 16
_start:
    mov %rsp, %rsi
    jmp main
.size _start, . - _start
