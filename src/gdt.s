.global flush_gdt
.type flush_gdt, function
.align 16
flush_gdt:
    mov %esi, %ds
    mov %esi, %ss
    mov %esi, %es
    mov %esi, %fs
    mov %esi, %gs
    pop %rsi
    push %rdi
    push %rsi
    lretq
.size flush_gdt, . - flush_gdt
