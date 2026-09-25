#include "../include/types.h"
#include "uart.h"

static char buf[128];

static int uart_getc(void) {
    reg32_t *fr = (reg32_t*)0x09000018;
    reg32_t *dr = (reg32_t*)0x09000000;
    while (*fr & (1 << 4));
    return *dr & 0xFF;
}

static int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a - *b;
}

static void cmd_help(void) {
    uart_puts("commands:\n");
    uart_puts("  help    - show this\n");
    uart_puts("  clear   - clear screen\n");
    uart_puts("  version - show version\n");
    uart_puts("  reboot  - restart\n");
}

static void cmd_clear(void) {
    uart_puts("\033[2J\033[H");
}

static void cmd_reboot(void) {
    uart_puts("rebooting...\n");
    reg32_t *psci = (reg32_t*)0x08000000;
    ((void(*)(uint64_t, uint64_t))psci)(0x84000009, 0);
}

static void run_cmd(char *cmd) {
    while (*cmd == ' ') cmd++;
    if (!*cmd) return;
    if (strcmp(cmd, "help") == 0) cmd_help();
    else if (strcmp(cmd, "clear") == 0) cmd_clear();
    else if (strcmp(cmd, "version") == 0) uart_puts("thunderz v0.1.0\n");
    else if (strcmp(cmd, "reboot") == 0) cmd_reboot();
    else { uart_puts("unknown: "); uart_puts(cmd); uart_puts("\n"); }
}

void kernel_main(void) {
    uart_init();
    uart_puts("\033[2J\033[H");
    uart_puts("thunderz v0.1.0\n");
    uart_puts("type 'help' for commands\n\n");
    uart_puts("thunderz> ");

    int pos = 0;
    for (;;) {
        int c = uart_getc();
        if (c == '\r' || c == '\n') {
            uart_puts("\n");
            buf[pos] = 0;
            run_cmd(buf);
            pos = 0;
            uart_puts("thunderz> ");
        } else if (c == 127 || c == 8) {
            if (pos > 0) { pos--; uart_puts("\b \b"); }
        } else if (c >= 32 && c < 127 && pos < 127) {
            buf[pos++] = c;
            uart_putc(c);
        }
    }
}