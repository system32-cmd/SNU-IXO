format pe64 dll efi
entry main

section '.text' code readable executable

main:
    ; Save the ImageHandle and SystemTable pointers passed by the firmware
    mov [ImageHandle], rcx
    mov [SystemTable], rdx

    ; Clear the screen to start fresh
    ; SystemTable->ConOut->ClearScreen(SystemTable->ConOut)
    mov rcx, [SystemTable]
    mov rcx, [rcx + 64]      ; ConOut pointer (Offset 64)
    mov rax, [rcx + 48]      ; ClearScreen function pointer (Offset 48)
    sub rsp, 32              ; Shadow space for Microsoft x64 calling convention
    call rax
    add rsp, 32

menu_loop:
    ; Print the Title
    mov rdx, msg_title
    call print_string

    ; Print Option 1
    mov rdx, msg_opt1
    call print_string

    ; Print Option 2
    mov rdx, msg_opt2
    call print_string

    ; Print Prompt
    mov rdx, msg_prompt
    call print_string

wait_key:
    ; Wait for a keypress
    ; SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &KeyData)
    mov rcx, [SystemTable]
    mov rcx, [rcx + 32]      ; ConIn pointer (Offset 32)
    lea rdx, [KeyData]       ; Pointer to output structure
    mov rax, [rcx + 8]       ; ReadKeyStroke function pointer (Offset 8)
    sub rsp, 32
    call rax
    add rsp, 32

    ; Check if ReadKeyStroke returned success (0)
    test rax, rax
    jnz wait_key             ; If error or no key, try again

    ; Check which key was pressed
    ; KeyData structure: UnicodeChar (WORD at offset 2)
    movzx eax, word [KeyData + 2]

    cmp eax, '1'
    je boot_kernel_selected

    cmp eax, '2'
    je reboot_selected

    jmp menu_loop            ; Invalid key, redraw / loop

boot_kernel_selected:
    mov rdx, msg_booting
    call print_string
    
    ; --- YOUR KERNEL LOADING CODE GOES HERE ---
    ; In a full loader, you would use SystemTable->BootServices->OpenFile
    ; to load your 'kernel.bin' into memory, then jump to its entry point.
    
    jmp $                    ; Halt for now

reboot_selected:
    ; SystemTable->RuntimeServices->ResetSystem(EfiResetCold, 0, 0, NULL)
    mov rcx, [SystemTable]
    mov rcx, [rcx + 88]      ; RuntimeServices pointer (Offset 88)
    mov rax, [rcx + 104]     ; ResetSystem function pointer (Offset 104)
    
    xor ecx, ecx             ; 0 = EfiResetCold
    xor edx, edx             ; Status = 0
    xor r8d, r8d             ; DataSize = 0
    xor r9d, r9d             ; ResetData = NULL
    sub rsp, 32
    call rax
    add rsp, 32
    jmp $

; --- Helper Function: Print String ---
; Input: RDX = Pointer to UTF-16 (wide) null-terminated string
print_string:
    push rbp
    mov rbp, rsp
    sub rsp, 32              ; Shadow space

    mov rcx, [SystemTable]
    mov rcx, [rcx + 64]      ; ConOut pointer
    mov rax, [rcx + 8]       ; OutputString function pointer (Offset 8)
    ; RCX is already ConOut, RDX is already the string pointer
    call rax

    mov rsp, rbp
    pop rbp
    ret

section '.data' data readable writeable

align 8
ImageHandle dq 0
SystemTable dq 0

; UEFI uses 16-bit Unicode (UTF-16) strings. FASM's 'du' defines user data as wide chars.
; 13, 10 is CR/LF for newlines.
msg_title   du 13, 10, '=== GRUB-Like FASM Bootloader ===', 13, 10, 0
msg_opt1    du 13, 10, ' [1] Boot Custom Kernel (kernel.bin)', 0
msg_opt2    du 13, 10, ' [2] Reboot Machine', 13, 10, 0
msg_prompt  du 13, 10, ' Select an option: ', 0
msg_booting du 13, 10, 'Loading kernel.bin...', 13, 10, 0

align 4
KeyData:
    .ScanCode    dw 0
    .UnicodeChar dw 0

section '.reloc' fixups data readable discardable
