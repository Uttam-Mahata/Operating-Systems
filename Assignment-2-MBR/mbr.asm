BITS 16          ; Set 16-bit assembly mode
ORG 0x7C00       ; Set the origin (start address) to 0x7C00

jmp short start  ; Short jump to 'start' label

; Padding with 0s up to byte 510
times 510 - ($ - $$) db 0

start:
    ; You can add boot code here (if needed)

    dw 0xAA55    ; Write the boot signature (little-endian: 0x55 0xAA)


; Padding the remaining to make it exactly 512 bytes
times 512 - ($ - $$) db 0