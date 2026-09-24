.section .text.boot, "ax"
.global _start
_start:
    mov x0, sp
    bl kernel_main
    b .