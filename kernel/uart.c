#include "../include/types.h"
#include "uart.h"

#define UART_BASE 0x09000000

#define UART_DR     ((reg32_t*)(UART_BASE + 0x00))
#define UART_FR     ((reg32_t*)(UART_BASE + 0x18))
#define UART_IBRD   ((reg32_t*)(UART_BASE + 0x24))
#define UART_FBRD   ((reg32_t*)(UART_BASE + 0x28))
#define UART_LCRH   ((reg32_t*)(UART_BASE + 0x2C))
#define UART_CR     ((reg32_t*)(UART_BASE + 0x30))
#define UART_IFLS   ((reg32_t*)(UART_BASE + 0x34))
#define UART_IMSC   ((reg32_t*)(UART_BASE + 0x38))

#define UART_FR_TXFF (1 << 5)
#define UART_FR_RXFE (1 << 4)

void uart_init(void) {
    *UART_CR = 0;
    *UART_IBRD = 26;
    *UART_FBRD = 3;
    *UART_LCRH = (3 << 5);
    *UART_CR = (1 << 0) | (1 << 8) | (1 << 9);
}

void uart_putc(char c) {
    while (*UART_FR & UART_FR_TXFF);
    *UART_DR = c;
}

void uart_puts(const char *s) {
    while (*s) {
        if (*s == '\n') uart_putc('\r');
        uart_putc(*s++);
    }
}