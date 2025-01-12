.code16
.global _start

.text
_start:
    // Set up segments
    xorw    %ax, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    movw    $0x7C00, %sp

    // Print message
    movw    $message, %si
    call    print_string

    // Infinite loop
    jmp     .

print_string:
    movb    $0x0E, %ah    // BIOS teletype output
1:  
    lodsb                  // Load next character
    testb   %al, %al      // Check if end of string
    jz      2f            // If zero, we're done
    int     $0x10         // Print character
    jmp     1b
2:  
    ret

message:
    .ascii  "Hello, OS World!"
    .byte   0

    // Fill with zeros until 510 bytes
    .org    510
    .word   0xAA55        // Boot signature
