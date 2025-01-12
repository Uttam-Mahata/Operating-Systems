.code16

.text
.global second_stage    # Add this line to make the label global

print_string:
    movb    $0x0E, %ah        # BIOS teletype function
1:  
    lodsb                     # Load byte from SI into AL
    testb   %al, %al         # Check if character is null
    jz      2f               # If null, return
    int     $0x10            # Print character
    jmp     1b               # Repeat for next character
2:  
    ret

second_stage:
    # Print second stage message
    movw    $stage2_msg, %si
    call    print_string

    # Example: Change color/display mode
    movb    $0x00, %ah    # Set video mode
    movb    $0x03, %al    # 80x25 text mode
    int     $0x10

    # Print colored message
    movb    $0x0E, %ah    # BIOS teletype
    movb    $0x02, %bl    # Green color
    movw    $colored_msg, %si
1:
    lodsb
    testb   %al, %al
    jz      2f
    int     $0x10
    jmp     1b
2:
    # Infinite loop
    jmp     .

stage2_msg:
    .ascii  "Entered second stage!\r\n\0"
colored_msg:
    .ascii  "Running from additional sectors!\r\n\0"
