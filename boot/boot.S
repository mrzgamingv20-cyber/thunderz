#ifndef ENTRY
#define ENTRY kernel_main
#endif

.section .text.boot, "ax"
.global _start
_start:
    mov x0, #(3 << 20)
    msr cpacr_el1, x0
    isb

    ldr x0, =0x48000000
    mov sp, x0

    ldr x0, =_bss_start
    ldr x1, =_bss_end
1:
    cmp x0, x1
    b.ge 2f
    str xzr, [x0], #8
    b 1b
2:
    bl ENTRY
3:
    wfi
    b 3b
