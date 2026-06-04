org 0x0600
use16

BOOT_DRIVE_ADDR equ 0x7DF0
KERNEL_SECTORS equ 64
CODE_SEL equ 0x08
DATA_SEL equ 0x10

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7A00

    mov si, load_msg
    call print_string

    mov dl, [BOOT_DRIVE_ADDR]
    xor dh, dh
    xor ch, ch
    mov cl, 5
    mov ah, 0x02
    mov al, 32
    mov bx, 0
    mov ax, 0x1000
    mov es, ax
    int 0x13
    jc disk_error

    add bx, 32*512
    add cl, 32
    mov ah, 0x02
    mov al, 32
    int 0x13
    jc disk_error

    call clear_memory
    call enable_a20
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEL:protected_entry

print_string:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    mov bh, 0
    mov bl, 0x07
    int 0x10
    jmp print_string
.done:
    ret

clear_memory:
    mov ax, 0x1000
    mov es, ax
    xor di, di
    xor ax, ax
    mov cx, 0x8000
    rep stosw
    ret

enable_a20:
    in al, 0x92
    or al, 2
    out 0x92, al
    ret

protected_entry:
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9FC00
    jmp $

print_error:
    mov si, err_msg
    call print_string
    hlt
    jmp $

disk_error:
    mov si, err_msg
    call print_string
    hlt
    jmp $

load_msg db 0x0D, 0x0A, "SNU loader: loading kernel...", 0
err_msg db 0x0D, 0x0A, "SNU loader: disk error", 0

gdt_descriptor:
    dw gdt_end - gdt - 1
    dd gdt

gdt:
    dd 0
    dd 0
    dd 0x00CF9A00
    dd 0x00CF9200

gdt_end:

times 2048 - ($ - $$) db 0