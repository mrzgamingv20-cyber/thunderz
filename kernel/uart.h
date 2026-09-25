#ifndef UART_H
#define UART_H

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);
void uart_hex(unsigned long long val);
int uart_getc(void);

#endif