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

/* -------------------------------------------------------------------------
 * gets / fgets  — backed by the kernel's readline-capable read(0,...) syscall
 * ------------------------------------------------------------------------- */

char *gets(char *buf) {
    /* The kernel's sys_read for fd=0 now does full readline with echo and
     * backspace support.  It appends '\n' before returning. */
    int n = read(0, buf, 4095);
    if (n <= 0) { buf[0] = '\0'; return NULL; }
    buf[n] = '\0';
    /* strip trailing newline so behaviour matches standard gets() */
    if (n > 0 && buf[n-1] == '\n') buf[--n] = '\0';
    if (n > 0 && buf[n-1] == '\r') buf[--n] = '\0';
    return buf;
}

char *fgets(char *buf, int size, int fd) {
    if (size <= 0) return NULL;
    int n = read(fd, buf, (size_t)(size - 1));
    if (n <= 0) { buf[0] = '\0'; return NULL; }
    buf[n] = '\0';
    return buf;
}

/* -------------------------------------------------------------------------
 * vsscanf — parse a string according to a format
 * Supports: %d %i %u %x %X %s %c %% and optional * (suppress assignment)
 * ------------------------------------------------------------------------- */

static int sc_isspace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

int vsscanf(const char *str, const char *fmt, va_list ap) {
    const char *s = str;
    int count = 0;

    while (*fmt) {
        if (sc_isspace(*fmt)) {
            while (sc_isspace(*s)) s++;
            fmt++;
            continue;
        }

        if (*fmt != '%') {
            if (*s == *fmt) { s++; fmt++; }
            else break;
            continue;
        }

        fmt++;

        int suppress = 0;
        if (*fmt == '*') { suppress = 1; fmt++; }

        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') { width = width * 10 + (*fmt - '0'); fmt++; }

        char spec = *fmt++;

        if (spec == '%') {
            while (sc_isspace(*s)) s++;
            if (*s == '%') s++;
            else break;
            continue;
        }

        if (spec != 'c') { while (sc_isspace(*s)) s++; }
        if (!*s) break;

        if (spec == 'c') {
            int w = (width == 0) ? 1 : width;
            if (suppress) {
                for (int i = 0; i < w && *s; i++) s++;
            } else {
                char *dest = va_arg(ap, char *);
                for (int i = 0; i < w && *s; i++) *dest++ = *s++;
                count++;
            }
        } else if (spec == 's') {
            if (suppress) {
                int w = 0;
                while (*s && !sc_isspace(*s) && (width == 0 || w < width)) { s++; w++; }
            } else {
                char *dest = va_arg(ap, char *);
                int w = 0;
                while (*s && !sc_isspace(*s) && (width == 0 || w < width)) { *dest++ = *s++; w++; }
                *dest = '\0';
                count++;
            }
        } else if (spec == 'd' || spec == 'i' || spec == 'u' || spec == 'x' || spec == 'X') {
            int neg = 0;
            unsigned int val = 0;
            int digits = 0;
            int base = 10;

            if (spec == 'd' || spec == 'i') {
                if (*s == '-') { neg = 1; s++; }
                else if (*s == '+') s++;
            }
            if (spec == 'x' || spec == 'X') base = 16;
            if (spec == 'i') {
                if (*s == '0') {
                    s++;
                    if (*s == 'x' || *s == 'X') { base = 16; s++; }
                    else base = 8;
                } else base = 10;
            }

            int w = 0;
            while (*s && (width == 0 || w < width)) {
                char c = *s;
                int d;
                if (c >= '0' && c <= '9') d = c - '0';
                else if (c >= 'a' && c <= 'f') d = 10 + (c - 'a');
                else if (c >= 'A' && c <= 'F') d = 10 + (c - 'A');
                else break;
                if (d >= base) break;
                val = val * (unsigned int)base + (unsigned int)d;
                s++; w++; digits++;
            }

            if (digits == 0) break;

            if (!suppress) {
                if (spec == 'u' || spec == 'x' || spec == 'X')
                    *va_arg(ap, unsigned int *) = val;
                else
                    *va_arg(ap, int *) = neg ? -(int)val : (int)val;
                count++;
            }
        } else {
            break;
        }
    }

    return count;
}

int sscanf(const char *str, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsscanf(str, fmt, ap);
    va_end(ap);
    return n;
}

int scanf(const char *fmt, ...) {
    char line[512];
    int nr = read(0, line, sizeof(line) - 1);
    if (nr <= 0) return -1;
    line[nr] = '\0';
    va_list ap;
    va_start(ap, fmt);
    int n = vsscanf(line, fmt, ap);
    va_end(ap);
    return n;
}
