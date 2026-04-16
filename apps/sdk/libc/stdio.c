#include "stdio.h"

#include <stdbool.h>
#include <stdint.h>

#include "string.h"
#include "unistd.h"

int putchar(int c) {
    char ch = (char)c;
    return write(1, &ch, 1);
}

int puts(const char *s) {
    int n = (int)write(1, s, strlen(s));
    write(1, "\n", 1);
    return n + 1;
}

static int print_uint(unsigned int value, unsigned int base, bool upper) {
    char tmp[32];
    int len = 0;
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    if (value == 0) tmp[len++] = '0';
    while (value > 0 && len < (int)sizeof(tmp)) {
        tmp[len++] = digits[value % base];
        value /= base;
    }
    int out = 0;
    for (int i = len - 1; i >= 0; i--) out += write(1, &tmp[i], 1);
    return out;
}

int vprintf(const char *fmt, va_list ap) {
    int out = 0;
    while (*fmt) {
        if (*fmt != '%') {
            out += write(1, fmt, 1);
            fmt++;
            continue;
        }
        fmt++;
        switch (*fmt) {
            case '%':
                out += write(1, "%", 1);
                break;
            case 'c': {
                char c = (char)va_arg(ap, int);
                out += write(1, &c, 1);
                break;
            }
            case 's': {
                const char *s = va_arg(ap, const char *);
                if (!s) s = "(null)";
                out += write(1, s, strlen(s));
                break;
            }
            case 'd':
            case 'i': {
                int n = va_arg(ap, int);
                if (n < 0) {
                    out += write(1, "-", 1);
                    out += print_uint((unsigned int)(-n), 10, false);
                } else {
                    out += print_uint((unsigned int)n, 10, false);
                }
                break;
            }
            case 'u':
                out += print_uint(va_arg(ap, unsigned int), 10, false);
                break;
            case 'x':
                out += print_uint(va_arg(ap, unsigned int), 16, false);
                break;
            case 'X':
                out += print_uint(va_arg(ap, unsigned int), 16, true);
                break;
            case 'p': {
                void *v = va_arg(ap, void *);
                out += write(1, "0x", 2);
                out += print_uint((unsigned int)(uintptr_t)v, 16, false);
                break;
            }
            default:
                out += write(1, "%", 1);
                out += write(1, fmt, 1);
                break;
        }
        fmt++;
    }
    return out;
}

int printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vprintf(fmt, ap);
    va_end(ap);
    return n;
}
