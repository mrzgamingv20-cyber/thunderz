.section .text.boot, "ax"
.global _start
_start:
    ldr x0, =0x48000000
    mov sp, x0
    bl kernel_main
1:
    wfi
    b 1b