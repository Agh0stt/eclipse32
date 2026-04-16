; Stage 2 Bootloader - Eclipse32 OS
; Loaded at 0x8000, 16 sectors (8192 bytes)

BITS 16
ORG 0x8000

%define KERNEL_LBA_START  17
%define KERNEL_SECTORS    420
%define MMAP_ADDR         0x500
%define BOOT_INFO_ADDR    0x600
%define VBE_INFO_ADDR     0x7000

stage2_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [boot_drv], dl

    call cls
    mov si, msg_stage2
    call print_str

    call detect_memory
    call enable_a20

    mov si, msg_loading
    call print_str
    call load_kernel

    ; VBE disabled - using VGA text mode
    ; call setup_vbe

    mov si, msg_pmode
    call print_str

    lgdt [gdt_ptr]
    cli
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp dword 0x08:pm_entry

detect_memory:
    pusha
    mov di, MMAP_ADDR + 4
    xor ebx, ebx
    xor bp, bp
    mov edx, 0x0534D4150
    mov eax, 0xE820
    mov ecx, 24
    mov dword [es:di+20], 1
    int 0x15
    jc .fail
    cmp eax, 0x0534D4150
    jne .fail
.loop:
    test ebx, ebx
    jz .done
    inc bp
    add di, 24
    mov eax, 0xE820
    mov ecx, 24
    mov dword [es:di+20], 1
    int 0x15
    jc .done
    jmp .loop
.done:
    mov [MMAP_ADDR], bp
    popa
    ret
.fail:
    mov word [MMAP_ADDR], 0
    popa
    ret

enable_a20:
    mov ax, 0x2401
    int 0x15
    jnc .done
    call .wait
    mov al, 0xAD
    out 0x64, al
    call .wait
    mov al, 0xD0
    out 0x64, al
    call .read
    push ax
    call .wait
    mov al, 0xD1
    out 0x64, al
    call .wait
    pop ax
    or al, 2
    out 0x60, al
    call .wait
    mov al, 0xAE
    out 0x64, al
    call .wait
.done:
    ret
.wait:
    in al, 0x64
    test al, 2
    jnz .wait
    ret
.read:
    in al, 0x64
    test al, 1
    jz .read
    in al, 0x60
    ret

load_kernel:
    mov dword [dap.lba_lo], KERNEL_LBA_START
    mov dword [dap.lba_hi], 0
    mov word  [dap.count],  127
    mov word  [dap.off],    0x0000
    mov word  [dap.seg],    0x1000
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drv]
    int 0x13
    jc .done

    mov dword [dap.lba_lo], KERNEL_LBA_START + 127
    mov dword [dap.lba_hi], 0
    mov word  [dap.count],  127
    mov word  [dap.off],    0xFE00
    mov word  [dap.seg],    0x1000
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drv]
    int 0x13
    jc .done

    mov dword [dap.lba_lo], KERNEL_LBA_START + 254
    mov dword [dap.lba_hi], 0
    mov word  [dap.count],  127
    mov word  [dap.off],    0x1C00
    mov word  [dap.seg],    0x2E00
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drv]
    int 0x13
    jc .done

    mov dword [dap.lba_lo], KERNEL_LBA_START + 381
    mov dword [dap.lba_hi], 0
    mov word  [dap.count],  39
    mov word  [dap.off],    0x1A00
    mov word  [dap.seg],    0x3E00
    mov si, dap
    mov ah, 0x42
    mov dl, [boot_drv]
    int 0x13
.done:
    ret

setup_vbe:
    pusha
    mov ax, 0x4F01
    mov cx, 0x0118
    mov di, VBE_INFO_ADDR
    int 0x10
    cmp ax, 0x004F
    jne .no_vbe
    mov ax, [VBE_INFO_ADDR]
    test ax, 0x0080
    jz .no_vbe
    mov ax, 0x4F02
    mov bx, 0x4118
    int 0x10
    cmp ax, 0x004F
    jne .no_vbe
    mov si, msg_vbe_ok
    call print_str
    popa
    ret
.no_vbe:
    mov word [VBE_INFO_ADDR], 0
    mov si, msg_vbe_fail
    call print_str
    popa
    ret

cls:
    mov ax, 0x0003
    int 0x10
    ret

print_str:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    xor bh, bh
    int 0x10
    jmp print_str
.done:
    ret

msg_stage2:   db "Eclipse32 Stage2", 0x0D, 0x0A, 0
msg_loading:  db "Loading kernel...", 0x0D, 0x0A, 0
msg_pmode:    db "Entering pmode...", 0x0D, 0x0A, 0
msg_vbe_ok:   db "VBE 800x600x24 OK", 0x0D, 0x0A, 0
msg_vbe_fail: db "VBE failed, text mode", 0x0D, 0x0A, 0

align 8
gdt_start:
    dq 0x0000000000000000
    dq 0x00CF9A000000FFFF
    dq 0x00CF92000000FFFF
gdt_end:

gdt_ptr:
    dw gdt_end - gdt_start - 1
    dd gdt_start

dap:
    db 0x10, 0
.count:  dw 0
.off:    dw 0
.seg:    dw 0
.lba_lo: dd 0
.lba_hi: dd 0

boot_drv: db 0x80

BITS 32
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9F000

    ; Copy kernel from 0x10000 to 0x100000
    mov esi, 0x10000
    mov edi, 0x100000
    mov ecx, (KERNEL_SECTORS * 512) / 4
    rep movsd

    ; Write boot_info at 0x600
    ; boot_info_t layout:
    ;   +0  magic        (dword)
    ;   +4  vbe_fb       (dword)
    ;   +8  vbe_width    (word)
    ;   +10 vbe_height   (word)
    ;   +12 vbe_pitch    (word)
    ;   +14 vbe_bpp      (byte)
    ;   +15 pad          (byte)
    ;   +16 mem_lower    (word)
    ;   +18 mem_upper    (dword)
    ;   +22 gdt_addr     (dword)
    ;   +26 boot_drive   (byte)

    mov dword [BOOT_INFO_ADDR +  0], 0xEC320001
    mov dword [BOOT_INFO_ADDR +  4], 0
    mov word  [BOOT_INFO_ADDR +  8], 0
    mov word  [BOOT_INFO_ADDR + 10], 0
    mov word  [BOOT_INFO_ADDR + 12], 0
    mov byte  [BOOT_INFO_ADDR + 14], 0
    mov byte  [BOOT_INFO_ADDR + 15], 0
    mov word  [BOOT_INFO_ADDR + 16], 640
    mov dword [BOOT_INFO_ADDR + 18], 0x20000
    mov dword [BOOT_INFO_ADDR + 22], 0
    mov byte  [BOOT_INFO_ADDR + 26], 0x80

    ; VBE disabled - framebuffer fields stay zero
    ; (VBE mode was not set, so no fields to populate)
    jmp .boot

.no_fb:

.boot:
    ; Pass boot_info pointer in EAX - kernel_entry_asm expects it there
    mov eax, BOOT_INFO_ADDR
    jmp 0x100000

    cli
.halt:
    hlt
    jmp .halt

BITS 16
TIMES (16*512)-($-$$) db 0
