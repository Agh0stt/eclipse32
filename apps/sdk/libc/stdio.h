#pragma once

#include <stdarg.h>
#include <stddef.h>

int putchar(int c);
int puts(const char *s);
int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);

/* Input */
char *gets(char *buf);
char *fgets(char *buf, int size, int fd);

/* Scanning */
int vsscanf(const char *str, const char *fmt, va_list ap);
int sscanf(const char *str, const char *fmt, ...);
int scanf(const char *fmt, ...);
