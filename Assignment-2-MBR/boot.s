/* filepath: /home/uttam/OS/boot.s */

/* We are in 16-bit real mode, so we need to tell the assembler to generate 16-bit code. */
.code16
/* Make the _start symbol visible to the linker. */
.global _start

.text
_start:
    /* Initialize segment registers to 0. This is a common practice to ensure
     * that our memory addressing is consistent. We can't move a value directly
     * into a segment register, so we use a general-purpose register (ax) as an
     * intermediate.
     * The 'w' suffix on the instructions indicates that we are working with words (16 bits).
     */
    xorw    %ax, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    /* Set up the stack pointer. It's good practice to set up a stack early on.
     * We'll point it to the beginning of our code, since we're not using it yet.
     */
    movw    $0x7C00, %sp

    /* Load the address of our message into the si register and call the print_string function. */
    movw    $message, %si
    call    print_string

    /* Hang the system. This is an infinite loop that jumps to the current address. */
    jmp     .

/* A function to print a null-terminated string to the screen.
 * Input: %si - pointer to the string
 */
print_string:
    /* Set the video mode to teletype output. This is a BIOS interrupt function. */
    movb    $0x0E, %ah
1:  /* This is a local label. */
    /* Load the next byte from the string (from address in si) into al and increment si. */
    lodsb
    /* Check if the byte is 0 (null terminator). */
    testb   %al, %al
    /* If it is, we're done. Jump forward to the next local label '2'. */
    jz      2f
    /* If not, print the character using the BIOS teletype interrupt. */
    int     $0x10
    /* Loop to the next character. Jump backward to the previous local label '1'. */
    jmp     1b
2:  /* This is another local label. */
    /* Return from the function. */
    ret

/* The message to be printed. It is null-terminated. */
message:
    .ascii  "Hello, OS World!"
    .byte   0

    /* Pad the rest of the file with zeros until we reach the 510th byte. */
    .org    510
    /* The boot signature. The BIOS requires this to be at the end of the boot sector.
     * It must be 0xAA55.
     */
    .word   0xAA55