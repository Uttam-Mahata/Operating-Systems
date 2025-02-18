; filepath: /home/uttam/OS/boot.asm
    org 0x7C00      ; BIOS loads boot sector to this address
    bits 16         ; Use 16-bit mode

start:
    ; Set up segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; Print message
    mov si, message
    call print_string

    ; Infinite loop
    jmp $

print_string:
    mov ah, 0x0E    ; BIOS teletype output
.loop:
    lodsb           ; Load next character
    test al, al     ; Check if end of string (0)
    jz .done        ; If zero, we're done
    int 0x10        ; Print character
    jmp .loop
.done:
    ret

message: db 'Hello, OS World!', 0

    ; Fill with zeros until 510 bytes
    times 510-($-$$) db 0
    ; Boot signature
    dw 0xAA55