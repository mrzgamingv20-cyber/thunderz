#include "../include/types.h"
#include "uart.h"

void kernel_main(void) {
    uart_init();
    uart_puts("thunderz v0.1.0\n");
    uart_puts("thunderz> ");
    
    for (;;) {
        asm volatile("wfi");
    }
}