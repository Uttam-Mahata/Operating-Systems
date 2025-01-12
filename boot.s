/* filepath: /home/uttam/OS/boot.s */
.code16
.global _start

.text
_start:
    # Set up segments
    xorw    %ax, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    movw    $0x7C00, %sp

    # Print first stage message
    movw    $stage1_msg, %si
    call    print_string

    # Load additional sectors
    movb    $0x02, %ah    # BIOS read sector function
    movb    $2, %al       # Number of sectors to read
    movb    $0x00, %ch    # Cylinder 0
    movb    $0x02, %cl    # Start from sector 2
    movb    $0x00, %dh    # Head 0
    movb    $0x00, %dl    # Drive 0 (floppy)
    movw    $0x7E00, %bx  # Load to ES:BX = 0x7E00
    int     $0x13
    jc      error         # If carry flag set, there was an error

    # Jump to second stage
    jmp     second_stage

error:
    movw    $error_msg, %si
    call    print_string
    jmp     .

print_string:
    movb    $0x0E, %ah
1:  
    lodsb
    testb   %al, %al
    jz      2f
    int     $0x10
    jmp     1b
2:  
    ret

stage1_msg:
    .ascii  "First stage bootloader...\r\n\0"
error_msg:
    .ascii  "Error loading sectors!\r\n\0"

    .org    510
    .word   0xAA55