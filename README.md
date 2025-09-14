# Operating Systems Assignments

This repository contains a series of assignments for an operating systems course. Each assignment is in its own directory and covers a different topic in operating systems.

## Getting Started

### Prerequisites

To compile and run the code in this repository, you will need the following:

*   A C compiler, such as `gcc`.
*   The `make` build automation tool.
*   For `Assignment-2-MBR`, you will also need `nasm` and `qemu`.

### Compilation

Most of the assignments can be compiled using the `make` command in their respective directories. For example, to compile the code in `Assignment-2-MBR`, you would run:

```bash
cd Assignment-2-MBR
make
```

For assignments that do not have a `Makefile`, you can compile the C files individually using `gcc`. For example:

```bash
gcc -o myprogram myprogram.c
```

## Assignments

*   **Assignment-2-MBR:** An exploration of the Master Boot Record (MBR) and the boot process.
*   **Assignment-3-FORK:** Exercises using the `fork()` system call to create new processes.
*   **Assignment-4-EXEC-SHM:** Assignments covering the `exec()` family of functions and shared memory.
*   **Assignment-5-IPC:** A client-server application demonstrating inter-process communication.
*   **Assignment-6-Semaphore:** Synchronization problems solved using semaphores.
*   **Assignment-7-Threads:** Exercises in multi-threading.
*   **Assignment-8-File-Management:** A simple file system implementation.
*   **sharedDS:** A shared data structure implementation.

## Usage

Each assignment directory contains its own `README.md` with detailed instructions on how to compile and run the code. Please refer to the `README.md` in each directory for specific instructions.

In general, after compiling the code, you can run the executables from the command line. For example:

```bash
./myprogram
```

Some programs may require command-line arguments. For example:

```bash
./problem1 hello world
```

## Contributing

This repository is for educational purposes and is not open to contributions.
