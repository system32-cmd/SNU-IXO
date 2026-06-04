org 0x7c00
use16

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00

    mov [0x7DF0], dl

    mov si, boot_msg
    call print_string

    mov ah, 0x02
    mov al, 4
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [0x7DF0]
    mov bx, 0x0600
    int 0x13
    jc disk_error

    jmp 0x0000:0x0600

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

disk_error:
    mov si, err_msg
    call print_string
    hlt
    jmp $

boot_msg db "SNU Apex Filesystem Boot", 0x0D, 0x0A, 0
err_msg db "Disk read error. Halting.", 0

times 510 - ($ - $$) db 0
dw 0xAA55