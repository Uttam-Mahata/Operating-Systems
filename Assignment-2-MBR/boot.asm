; filepath: /home/uttam/OS/boot.asm
    ; Tell the assembler where this code will be loaded in memory.
    ; 0x7C00 is the standard address where the BIOS loads the boot sector.
    org 0x7C00
    ; We are in 16-bit real mode, so we need to tell the assembler to generate 16-bit code.
    bits 16

start:
    ; Initialize segment registers to 0. This is a common practice to ensure
    ; that our memory addressing is consistent. We can't move a value directly
    ; into a segment register, so we use a general-purpose register (ax) as an
    ; intermediate.
    xor ax, ax      ; ax = 0
    mov ds, ax      ; Data Segment = 0
    mov es, ax      ; Extra Segment = 0
    mov ss, ax      ; Stack Segment = 0

    ; Set up the stack pointer. It's good practice to set up a stack early on.
    ; We'll point it to the beginning of our code, since we're not using it yet.
    mov sp, 0x7C00

    ; Load the address of our message into the si register and call the print_string function.
    mov si, message
    call print_string

    ; Hang the system. This is an infinite loop that jumps to itself.
    jmp $

; A function to print a null-terminated string to the screen.
; Input: si - pointer to the string
print_string:
    ; Set the video mode to teletype output. This is a BIOS interrupt function.
    mov ah, 0x0E
.loop:
    ; Load the next byte from the string (from address in si) into al and increment si.
    lodsb
    ; Check if the byte is 0 (null terminator).
    test al, al
    ; If it is, we're done.
    jz .done
    ; If not, print the character using the BIOS teletype interrupt.
    int 0x10
    ; Loop to the next character.
    jmp .loop
.done:
    ; Return from the function.
    ret

; The message to be printed. It is null-terminated.
message: db 'Hello, OS World!', 0

    ; Pad the rest of the file with zeros until we reach the 510th byte.
    ; The '$' symbol represents the current address, and '$$' represents the start address of the section.
    times 510-($-$$) db 0
    ; The boot signature. The BIOS requires this to be at the end of the boot sector.
    ; It must be 0xAA55.
    dw 0xAA55