format pe64 dll efi
entry main

section '.text' code readable executable

CODE_SEL equ 0x08
DATA_SEL equ 0x10

main:
    mov [ImageHandle], rcx
    mov [SystemTable], rdx

    call clear_screen
    mov rdx, msg_title
    call print_string

    mov rcx, [SystemTable]
    mov rcx, [rcx + 96]          ; BootServices pointer
    mov [BootServices], rcx

    lea rdx, [gEfiSimpleFileSystemProtocolGuid]
    lea r8, [FileSystem]
    mov rcx, [BootServices]
    mov rax, [rcx + 0x88]        ; HandleProtocol
    sub rsp, 32
    call rax
    add rsp, 32
    test rax, rax
    jne boot_failed

    mov rcx, [FileSystem]
    lea rdx, [RootDir]
    mov rax, [rcx + 8]           ; OpenVolume
    sub rsp, 32
    call rax
    add rsp, 32
    test rax, rax
    jne boot_failed

    mov rcx, [RootDir]
    lea rdx, [KernelPath]
    mov r8, [KernelOpenMode]
    xor r9, r9
    sub rsp, 32
    mov qword [rsp], 0
    mov rax, [rcx + 8]           ; Open
    call rax
    add rsp, 32
    test rax, rax
    jne boot_failed

    mov rcx, [BootServices]
    mov rdx, 2                   ; EfiAllocateAddress
    mov r8, 2                   ; EfiLoaderData
    mov r9, 8                   ; 8 pages = 32KiB
    lea r10, [KernelAddr]
    mov rax, [rcx + 0x18]        ; AllocatePages
    sub rsp, 32
    call rax
    add rsp, 32
    test rax, rax
    jne boot_failed

    mov rcx, [KernelFile]
    lea rdx, [KernelSize]
    mov r8, [KernelAddr]
    mov rax, [rcx + 0x20]        ; Read
    sub rsp, 32
    call rax
    add rsp, 32
    test rax, rax
    jne boot_failed

    mov rdx, msg_loaded
    call print_string

    lgdt [GDTDescriptor]
    push word 0x08
    lea rax, [protected_entry]
    push rax
    retfq

protected_entry:
    use32
    mov ax, DATA_SEL
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x9FC00
    mov eax, dword [KernelAddr]
    jmp eax

    use64
boot_failed:
    mov rdx, msg_failed
    call print_string
    hlt
    jmp $

clear_screen:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    mov rcx, [SystemTable]
    mov rcx, [rcx + 64]          ; ConOut
    mov rax, [rcx + 48]          ; ClearScreen
    call rax
    mov rsp, rbp
    pop rbp
    ret

print_string:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    mov rcx, [SystemTable]
    mov rcx, [rcx + 64]          ; ConOut
    mov rax, [rcx + 8]           ; OutputString
    call rax
    mov rsp, rbp
    pop rbp
    ret

section '.data' data readable writeable

align 8
ImageHandle dq 0
SystemTable dq 0
BootServices dq 0
FileSystem dq 0
RootDir dq 0
KernelFile dq 0
KernelAddr dq 0
KernelSize dq 0
KernelOpenMode dq 1

; GUID for EFI_SIMPLE_FILE_SYSTEM_PROTOCOL: 09576e91-6d3f-11d2-8e39-00a0c969723b
align 8
gEfiSimpleFileSystemProtocolGuid:
    dd 0x09576e91
    dw 0x6d3f
    dw 0x11d2
    db 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b

align 2
KernelPath:
    dw 'k', 'e', 'r', 'n', 'e', 'l', '.', 'b', 'i', 'n', 0

align 2
msg_title   du 13, 10, '=== SNU UEFI Loader ===', 13, 10, 0
msg_loaded  du 13, 10, 'kernel.bin loaded, jumping to OS...', 13, 10, 0
msg_failed  du 13, 10, 'UEFI loader failed to boot kernel.bin', 13, 10, 0

section '.const' data readable

align 8
GDTDescriptor:
    dw gdt_end - gdt - 1
    dq gdt

gdt:
    dq 0
    dq 0x00CF9A000000FFFF    ; code segment, 32-bit, readable, accessed=0
    dq 0x00CF92000000FFFF    ; data segment, 32-bit, accessed=0

gdt_end:

section '.reloc' fixups data readable discardable
