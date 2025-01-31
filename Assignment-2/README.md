# Simple Bootloader Project (GNU AS Version)

## Project Structure
```
.
├── boot.s      # Main bootloader GNU assembly code
├── Makefile    # Build automation script
└── README.md   # Project documentation
```

// ...existing code...

### 1. Assembly Code (boot.s)

#### Key Components:
- `.code16`: Sets 16-bit mode
  - Equivalent to NASM's `bits 16`
  
- `.global _start`: Makes entry point visible to linker
  
- Register Setup:
  ```gas
  xorw    %ax, %ax
  movw    %ax, %ds
  ```
  - AT&T syntax uses `%` for registers
  - Source and destination order is reversed from NASM

// ...existing code...

### 3. Commands Explained

#### GNU Assembler (as) Command:
```bash
as -o boot.o boot.s
```
- Creates object file from assembly
- AT&T syntax by default

#### GNU Linker (ld) Command:
```bash
ld -Ttext 0x7C00 --oformat binary -o boot.bin boot.o
```
- `-Ttext 0x7C00`: Sets code start address
- `--oformat binary`: Creates raw binary output

// ...rest of existing README content...

## Key Differences from NASM
1. AT&T Syntax vs Intel Syntax:
   - Destination comes last (opposite of NASM)
   - Registers prefixed with %
   - Immediate values prefixed with $
   
2. Directives:
   - `.ascii` instead of `db`
   - `.word` instead of `dw`
   - `.org` instead of `times`

3. Labels:
   - Local labels use numbers (1:, 2:)
   - Global labels need `.global` directive
