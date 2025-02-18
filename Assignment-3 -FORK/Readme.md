# Operating Systems Lab Assignment 3 - Process Management

This repository contains five C programs demonstrating various aspects of process management using fork() and file operations in Unix/Linux systems.

## Programs Overview

### 1. String Reversal with Multiple Processes (problem1.c)
- Takes command line arguments as input strings
- Creates a child process for each input string
- Each child process reverses and prints its assigned string
- Demonstrates basic process creation and command line argument handling

### 2. Finding K-th Largest Numbers (problem2.c)
- Accepts an array of integers from user input
- Sorts the array using Bubble Sort
- Creates N child processes (N = array size)
- Each child process prints the i-th largest number
- Shows process creation with sorting algorithms

### 3. Sorting with Exit Status (problem3.c)
- Enhanced version of problem2.c
- Uses exit status to pass values from child to parent
- Creates a sorted array using child process exit values
- Demonstrates inter-process communication using exit codes

### 4. File Reading with Multiple Processes (problem4.c)
- Creates a test file with multiple lines
- Demonstrates file sharing between parent and child processes
- Shows how file pointers work across process boundaries
- Illustrates file position indicators in multiple processes

### 5. File Writing with Multiple Processes (problem5.c)
- Shows concurrent file writing from parent and child processes
- Demonstrates file position tracking
- Includes sleep delays to show timing effects
- Displays final file contents after all operations

## Key Concepts Demonstrated
- Process Creation using fork()
- File Operations
- Inter-Process Communication
- Process Synchronization
- File Descriptors and Position Indicators

## How to Compile and Run

```bash
# Compile any program
gcc -o program programN.c

# Run problem1 with command line arguments
./program1 string1 string2 string3

# Run other programs
./program2
./program3
./program4
./program5
```

## Important Notes
- Each program demonstrates different aspects of process management
- File operations are demonstrated in problem4 and problem5
- Programs use proper error handling for fork() and file operations
- Memory management is implemented where necessary