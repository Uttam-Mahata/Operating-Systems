# Fork and Exec Examples

This repository contains three examples demonstrating the usage of fork() and exec() system calls in C.

## Programs Overview

### 1. Basic Fork Example (forkNexec1.c)
- Demonstrates basic parent-child process creation
- Takes string input from user
- Child process prints the string
- Parent process waits for child and shows exit status
- Runs in an infinite loop until terminated

### 2. Enhanced Fork Example (forkNexec2.c)
- Similar to example 1 but with enhanced output
- Child process explicitly indicates it's printing the string
- Parent process provides more detailed status information
- Demonstrates basic process synchronization

### 3. Fork with Exec Example (forkNexec3.c)
- Advanced example combining fork() and execve()
- Takes executable filename as input
- Child process attempts to execute the specified program
- Parent process monitors execution status
- Handles execution failures and signals
- Shows how to replace child process image with new program

## How to Compile and Run

```bash
# Compile the programs
gcc forkNexec1.c -o forkNexec1
gcc forkNexec2.c -o forkNexec2
gcc forkNexec3.c -o forkNexec3

# Run the programs
./forkNexec1
./forkNexec2
./forkNexec3
```

## Key Features
- Process creation using fork()
- Process execution using execve()
- Process synchronization using wait()
- Error handling and status reporting
- Signal handling (in forkNexec3.c)

## Notes
- All programs run in infinite loops
- Use Ctrl+C to terminate
- For forkNexec3.c, provide valid executable paths
- Exit status 9 is used as a success indicator in examples