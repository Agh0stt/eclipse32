#pragma once

#include "../kernel.h"

typedef struct {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_dummy;
    uint32_t ebx, edx, ecx, eax;
    uint32_t int_num, err_code;
    uint32_t eip, cs, eflags;
    uint32_t user_esp, user_ss;
} PACKED isr_regs_t;

enum {
    SYS_write = 1,
    SYS_read = 2,
    SYS_open = 3,
    SYS_close = 4,
    SYS_seek = 5,
    SYS_fstat = 6,
    SYS_readfile = 7,
    SYS_exit = 8,
    SYS_mkdir = 9,
    SYS_unlink = 10,
    SYS_rename = 11,
    SYS_readdir = 12,
    SYS_gettime_ms = 13,
    SYS_sleep_ms = 14,
    SYS_brk = 15,
    SYS_getpid = 16,
    SYS_isatty = 17,
    SYS_getcwd = 18,
    SYS_chdir = 19,
};

#define SYS_ENOSYS  (-38)

typedef struct {
    uint32_t size;
    uint32_t is_dir;
} sys_stat_t;

typedef struct {
    char     name[256];
    uint32_t size;
    uint32_t cluster;
    uint32_t is_dir;
    uint32_t is_hidden;
} PACKED sys_dirent_t;

void syscall_init(void);
void syscall_dispatch_handler(void *regs);

void syscall_app_begin(void);
bool syscall_app_exit_requested(void);
int syscall_app_exit_code(void);
void syscall_set_app_image(const void *base, uint32_t size);
void syscall_set_app_heap(uint32_t brk_base, uint32_t brk_limit);
