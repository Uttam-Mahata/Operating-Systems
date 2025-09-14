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
# Compile the fork and exec programs
gcc forkNexec1.c -o forkNexec1
gcc forkNexec2.c -o forkNexec2
gcc forkNexec3.c -o forkNexec3
gcc forkNexec4.c -o forkNexec4

# Compile the shared memory programs
gcc factorial_shm.c -o factorial_shm
gcc matrix_multiplication_shm.c -o matrix_multiplication_shm
gcc sample_program_shm.c -o sample_program_shm

# Run the programs
./forkNexec1
./forkNexec2
./forkNexec3
./forkNexec4
./factorial_shm
./matrix_multiplication_shm
./sample_program_shm
```

## Shared Memory Programs

### 1. Factorial Calculation (factorial_shm.c)
- Demonstrates inter-process communication using shared memory.
- The parent process generates random numbers and passes them to the child process through shared memory.
- The child process calculates the factorial of the numbers and passes the result back to the parent, also through shared memory.

### 2. Matrix Multiplication (matrix_multiplication_shm.c)
- Demonstrates parallel matrix multiplication using shared memory and multiple child processes.
- The parent process reads two matrices from the user and stores them in shared memory.
- It then creates a child process for each row of the output matrix.
- Each child process calculates one row of the result matrix and stores it in shared memory.
- The parent process then waits for all child processes to complete and prints the result matrix.

### 3. Sample Program (sample_program_shm.c)
- A simple demonstration of shared memory.
- The parent process writes integers to a shared memory segment, and the child process reads them.

## Simple Shell

### 1. Simple Shell (forkNexec4.c)
- Implements a simple shell that can execute commands with arguments.
- It demonstrates the use of `fork()`, `execvp()`, and `wait()` to create a new process to execute a command.

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