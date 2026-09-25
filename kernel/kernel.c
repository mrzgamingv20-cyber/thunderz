#include "../include/types.h"
#include "../include/string.h"
#include "uart.h"
#include "blk.h"
#include "tfs.h"

static char buf[128];

static void print_num(uint64_t n) {
    char tmp[21];
    int i = 0;
    if (n == 0) { uart_putc('0'); return; }
    while (n) { tmp[i++] = '0' + n % 10; n /= 10; }
    while (i--) uart_putc(tmp[i]);
}

static void cmd_help(void) {
    uart_puts("commands:\n");
    uart_puts("  help     - show this\n");
    uart_puts("  clear    - clear screen\n");
    uart_puts("  version  - show version\n");
    uart_puts("  reboot   - restart\n");
    uart_puts("  disk     - disk info\n");
    uart_puts("  ls       - list files\n");
    uart_puts("  cat <f>  - read file\n");
    uart_puts("  write <f> <text> - write file\n");
}

static void cmd_clear(void) {
    uart_puts("\033[2J\033[H");
}

static void cmd_reboot(void) {
    uart_puts("rebooting...\n");
    reg32_t *psci = (reg32_t*)0x08000000;
    ((void(*)(uint64_t, uint64_t))psci)(0x84000009, 0);
}

static int disk_ok;

static void cmd_disk(void) {
    if (!disk_ok) { uart_puts("no disk\n"); return; }
    uart_puts("disk capacity: ");
    print_num(blk_capacity() / 2048);
    uart_puts(" MB\n");
}

static void ls_cb(const char *name, uint64_t size) {
    uart_puts("  ");
    uart_puts(name);
    uart_puts(" (");
    print_num(size);
    uart_puts(" bytes)\n");
}

static void cmd_ls(void) {
    if (tfs_list(ls_cb) < 0) uart_puts("not mounted\n");
}

static void cmd_cat(char *path) {
    if (!path || !*path) { uart_puts("usage: cat <file>\n"); return; }
    char dbuf[512];
    int n = tfs_read(path, dbuf, 511);
    if (n < 0) { uart_puts("not found\n"); return; }
    dbuf[n] = 0;
    uart_puts(dbuf);
    uart_puts("\n");
}

static void cmd_write(char *args) {
    if (!args || !*args) { uart_puts("usage: write <file> <text>\n"); return; }
    char *sp = args;
    while (*sp && *sp != ' ') sp++;
    if (!*sp) { uart_puts("usage: write <file> <text>\n"); return; }
    *sp++ = 0;
    int n = tfs_write(args, sp, strlen(sp));
    if (n < 0) uart_puts("write failed\n");
    else { uart_puts("wrote "); print_num(n); uart_puts(" bytes\n"); }
}

static void run_cmd(char *cmd) {
    while (*cmd == ' ') cmd++;
    if (!*cmd) return;

    if (strcmp(cmd, "help") == 0) cmd_help();
    else if (strcmp(cmd, "clear") == 0) cmd_clear();
    else if (strcmp(cmd, "version") == 0) uart_puts("thunderz v0.1.0\n");
    else if (strcmp(cmd, "reboot") == 0) cmd_reboot();
    else if (strcmp(cmd, "disk") == 0) cmd_disk();
    else if (strcmp(cmd, "ls") == 0) cmd_ls();
    else if (strncmp(cmd, "cat ", 4) == 0) cmd_cat(cmd + 4);
    else if (strncmp(cmd, "write ", 6) == 0) cmd_write(cmd + 6);
    else { uart_puts("unknown: "); uart_puts(cmd); uart_puts("\n"); }
}

void kernel_main(void) {
    uart_init();
    uart_puts("\033[2J\033[H");
    uart_puts("thunderz v0.1.0\n");

    uint32_t boot_mode = *(volatile uint32_t*)0x40FFF000;
    if (boot_mode == 1) {
        uart_puts("safe mode: disk skipped\n");
        disk_ok = 0;
    } else {
        uart_puts("probing virtio-blk...\n");
        disk_ok = blk_init() == 0;
        if (disk_ok) {
            uart_puts("virtio-blk: found disk (");
            print_num(blk_capacity() / 2048);
            uart_puts(" MB)\n");
            uart_puts("mounting tfs...\n");
            if (tfs_mount() == 0) uart_puts("tfs: mounted\n");
            else uart_puts("tfs: no filesystem\n");
        } else {
            uart_puts("virtio-blk: no disk\n");
        }
    }

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