#include "../include/types.h"
#include "../include/string.h"
#include "../kernel/uart.h"
#include "../kernel/blk.h"
#include "../kernel/tfs.h"

#define STAGE2_ADDR 0x41000000UL
#define BOOT_MODE_ADDR 0x40FFF000UL

void boot_main(void) {
    uart_init();
    uart_puts("\033[2J\033[H");
    uart_puts("thunderz bootloader v0.1\n\n");

    uint32_t mode = 0;
    for (;;) {
        uart_puts("1) boot thunderz\n");
        uart_puts("2) safe mode (kernel without disk)\n");
        uart_puts("\nchoice: ");
        int c = uart_getc();
        uart_putc(c);
        uart_puts("\n");
        if (c == '1') { mode = 0; break; }
        if (c == '2') { mode = 1; break; }
    }

    *(volatile uint32_t*)BOOT_MODE_ADDR = mode;

    uart_puts("loading kernel...\n");
    if (blk_init() < 0) { uart_puts("error: no disk\n"); goto halt; }
    if (tfs_mount() < 0) { uart_puts("error: no filesystem\n"); goto halt; }
    int n = tfs_read("kernel.bin", (void*)STAGE2_ADDR, 10240);
    if (n <= 0) { uart_puts("error: kernel.bin not found\n"); goto halt; }
    uart_puts("booting...\n\n");

    asm volatile("dsb sy");
    ((void(*)(void))STAGE2_ADDR)();

halt:
    uart_puts("\nhalted.\n");
    for (;;) asm volatile("wfi");
}
