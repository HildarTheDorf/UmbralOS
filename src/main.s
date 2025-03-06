.global flush_gdt
.type flush_gdt, function
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

.global interrupt_handler
.type interrupt_handler, function
interrupt_handler:
    iretq
.size interrupt_handler, . - interrupt_handler
